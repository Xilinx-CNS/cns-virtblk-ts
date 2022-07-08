#! /usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
from requests.api import request
from requests.models import Response
import jf
from typing import Any, Dict, List, Optional, Tuple, Set, Union
import os
import subprocess
import json
import sys


def get_path_to_artifactory(url: str) -> str:
    return url[1:]


def check_done_file(repo: str, path: str) -> bool:
    full_path = path + '/.done'
    if (jf.path_info(repo, path) == None):
        raise Exception(f'<p>The session {path} was not fully completed</p>')


def get_file(repo: str, path: str, location: str = None) -> Response:
    r = jf.download_path(repo, path, True)
    if (r is None):
        raise Exception(f'<p>Cannot get {path} from artifactory</p>')

    if location is not None:
        with open(location, 'wb') as file:
            file.write(r.raw.read())

    return r


def check_meta_data_file(repo: str, path: str) -> None:
    meta_data = get_file(repo, path + '/meta_data.json').json()
    for m in meta_data["metas"]:
        if m["name"] == 'PACKAGE' and m["value"] != 'performance' and m[
                "value"] != 'perf_short':
            raise Exception(
                '<p>Char can only be built for performance session</p>')

        if m["name"] == 'RUN_STATUS' and m["value"] != 'DONE':
            raise Exception(
                f'<p>The session {path} ended with errors</p>. <p>Charts can only be '
                f'built for successful sessions</p>')


def call_cmd(cmd: str) -> None:
    try:
        r = subprocess.run([cmd],
                           shell=True,
                           check=True,
                           stderr=subprocess.PIPE)
    except Exception as e:
        raise Exception(f'<p>Failed to execute {cmd}</p>'
                        f'<p>Error: {e.stderr}</p>')


def get_raw_log(bundle: str, raw_log: str, te_bin: str) -> None:
    cmd_raw_log = (
        f'{te_bin}/rgt-log-bundle-get-item --bundle={bundle} --req-path={raw_log}'
    )

    call_cmd(cmd_raw_log)


def get_mi_log(raw: str, mi: str, te_path: str, te_install_path: str) -> None:
    cmd_mi_log = (
        f'TE_INSTALL={te_install_path} {te_path}/scripts/log.sh --mi --raw-log={raw} --output-to={mi}'
    )

    call_cmd(cmd_mi_log)


def pull_files_from_artifactory(repo: str, path: str,
                                bundle_path: str) -> None:
    try:
        check_done_file(repo, path)
        check_meta_data_file(repo, path)
        get_file(repo, path + '/raw_log_bundle.tpxz', bundle_path)

    except Exception as e:
        raise Exception(f'<p>Failed to get files from artifactory</p>' f'{e}')


def create_mi_log(url: str, mi_log_path: str, log_cache_tmp: str) -> None:
    repo = 'virtblk-logs'
    bundle_path = log_cache_tmp + '/raw_log_bundle.tpxz'
    raw_log_path = log_cache_tmp + '/log.raw'
    te_install_path = '/home/xce-ci-virtblk/charts/te-build/inst'
    te_bin_path = te_install_path + '/default/bin'
    te_path = '/home/xce-ci-virtblk/charts/te'

    try:
        path = get_path_to_artifactory(url)
        pull_files_from_artifactory(repo, path, bundle_path)
        get_raw_log(bundle_path, raw_log_path, te_bin_path)
        get_mi_log(raw_log_path, mi_log_path, te_path, te_install_path)
    except Exception as e:
        raise Exception(f'<p>Failed to create MI log</p>' f'{e}')
    finally:
        if os.path.exists(bundle_path):
            os.remove(bundle_path)
        if os.path.exists(raw_log_path):
            os.remove(raw_log_path)


def call_perf_py(perf_path: str, mi_log_path: str, output: str,
                 cmd_param: str) -> Dict:
    cmd = (f'python3 -W ignore {perf_path} -i {mi_log_path} -j -o {output} ')
    cmd += cmd_param

    call_cmd(cmd)

    with open(output) as json_file:
        data = json.load(json_file)
    return data


def get_info_from_perf_py(graphs: List[Dict], log_cache_tmp: str,
                          perf_path: str, mi_log_path: str) -> None:
    perf_out_path = log_cache_tmp + '/perf.json'

    try:
        for g in graphs:
            d = call_perf_py(perf_path, mi_log_path, perf_out_path, g["cmd"])
            d["params"].remove("Plan-Name=fio_perf_iops_bw")
            g["params"] = d
    except Exception as e:
        raise Exception(f'<p>Failed to call {perf_path}</p>' f'{e}')
    finally:
        if os.path.exists(perf_out_path):
            os.remove(perf_out_path)


def build_graph(graphs: List[Dict]) -> str:
    i = 1
    body = ''
    for g in graphs:
        g["params"]["params"].sort()
        body += graph_script.format(
            xs_axis=g["params"]["x_points"],
            ys_axis=g["params"]["y_points"],
            labels=', '.join(map(str, g["params"]["params"])),
            title=g["name"],
            x_axis=g["params"]["x_axis"],
            y_axis=g["params"]["y_axis"],
            number=i,
        )
        i += 1

    return body


head = '''
<html lang="en">
<head>
  <title>Performance graph</title>
  <meta charset="utf-8">
  <link rel="stylesheet" href="/bootstrap.min.css">
  <link rel="stylesheet" href="/error.css">
  <script src="/Chart.bundle.min.js"> </script>
</head>
<body>
    {body}
</body>
</html>
'''

graph_container = '''
<div class="container-fluid">
  <h2 style="text-align: center;">CEPH performance</h2>
  <div class="row">
    <div class="col-md-6">
        <canvas id="myChart1" style="margin:auto;width:90%;max-width:85%"></canvas>
    </div>
    <div class="col-md-6">
        <canvas id="myChart2" style="margin:auto;width:90%;max-width:85%"></canvas>
    </div>
  </div>
  <br>
  <div class="row">
    <div class="col-md-6">
        <canvas id="myChart3" style="margin:auto;width:90%;max-width:85%"></canvas>
    </div>
    <div class="col-md-6">
        <canvas id="myChart4" style="margin:auto;width:90%;max-width:85%"></canvas>
    </div>
  </div>
</div>
'''

error_container = '''
<div class="contain">
	<div class="congrats">
		<h1>Congratulations! An error is occured!</h1>
        <br>
		<div class="done drawn">
			<svg version="1.1" id="tick" x="0px" y="0px"
	 viewBox="0 0 37 37" style="enable-background:new 0 0 37 37;" xml:space="preserve">
<path class="circ path" style="fill:#0cdcc7;stroke:#07a796;stroke-width:3;stroke-linejoin:round;stroke-miterlimit:10;" d="
	M30.5,6.5L30.5,6.5c6.6,6.6,6.6,17.4,0,24l0,0c-6.6,6.6-17.4,6.6-24,0l0,0c-6.6-6.6-6.6-17.4,0-24l0,0C13.1-0.2,23.9-0.2,30.5,6.5z"
	/>
<polyline class="tick path" style="fill:none;stroke:#fff;stroke-width:3;stroke-linejoin:round;stroke-miterlimit:10;" points="
	11.6,20 15.9,24.2 26.4,13.8 "/>
</svg>
			</div>
		<div class="text">
        {msg}
			</div>
		<p class="regards">Regards, virtblk-ts testing</p>
	</div>
</div>
'''

graph_script = '''
<script>
   var x_points = {xs_axis}
   var y_points = {ys_axis}

    for (let i = 0; i < Math.max(...x_points) + 1; i++) {{
        if (i != x_points[i]) {{
            x_points.splice(i,0,i)
            y_points.splice(i, 0, null)
        }}
    }}

    console.log(x_points)
    console.log(y_points)
    var data = {{
        labels: x_points,
        datasets: [{{
          label: "{labels}",
          data: y_points,
          fill: false,
          pointRadius: 6,
          pointBackgroundColor: "rgb(0,0,255)",
          stepped: true,
          borderColor: 'blue',
        }}]
    }}

    var options = {{
	spanGaps: true,
        legend: {{
            display: true,
            position: 'top',
            labels: {{
                boxWidth: 0,
                fontColor: 'black',
                fontSize: 14,
            }}
        }},
        title: {{
            display: true,
            text: '{title}',
            fontSize: 20,
        }},
        scales: {{
            xAxes: [{{
                scaleLabel: {{
                    display: true,
                    labelString: "{x_axis}",
                    fontSize: 18,
                }}
            }}],
            yAxes: [{{
                scaleLabel: {{
                    display: true,
                    labelString: "{y_axis}",
                    fontSize: 18,
                }},
                ticks : {{
                    min: 0
                }}
            }}]
        }}
    }}

    const chrt{number} = document.getElementById('myChart{number}');

    new Chart(chrt{number}, {{
      type: "line",
      data: data,
      options: options
    }});

</script>
'''

graphs = [
    {
        "name":
        'Small block read IOPS',
        "cmd":
        "-m \'Read iops\' \'Dev1-R-Threads\' Plan-Name=fio_perf_iops_bw/Dev1-Threads-Crossed=False/FIO-blocksize=4096/FIO-iodepth={64,16}/FIO-rwmixread=100/FIO-numjobs={4,16}"
    },
    {
        "name":
        'Small block write IOPS',
        "cmd":
        "-m \'Write iops\' \'Dev1-W-Threads\' Plan-Name=fio_perf_iops_bw/Dev1-Threads-Crossed=False/FIO-blocksize=4096/FIO-iodepth={64,16}/FIO-rwmixread=0/FIO-numjobs={4,16}"
    },
    {
        "name":
        'Large block read bandwidth',
        "cmd":
        "-m \'Read throughput\' \'Dev1-R-Threads\' Plan-Name=fio_perf_iops_bw/Dev1-Threads-Crossed=False/FIO-blocksize=1048576/FIO-iodepth={64,16}/FIO-rwmixread=100/FIO-numjobs={4,16}"
    },
    {
        "name":
        'Large block write bandwidth',
        "cmd":
        "-m \'Write throughput\' \'Dev1-W-Threads\' Plan-Name=fio_perf_iops_bw/Dev1-Threads-Crossed=False/FIO-blocksize=1048576/FIO-iodepth={64,16}/FIO-rwmixread=0/FIO-numjobs={4,16}"
    },
]

log_cache_tmp = '/wrk/te_logs_cache/log-cache/tmp'
#log_cache_tmp = '/home/okt-artm/cgi-bin/tmp'
mi_log_path = log_cache_tmp + '/mi.json'
perf_path = '/home/xce-ci-virtblk/charts/perf.py'

if __name__ == "__main__":
    body = ''
    try:
        print("Content-type: text/html\n\n")
        jf.disable_ssl_warnings()
        create_mi_log(os.environ['PATH_INFO'], mi_log_path, log_cache_tmp)
        get_info_from_perf_py(graphs, log_cache_tmp, perf_path, mi_log_path)

        body = graph_container
        body += build_graph(graphs)
    except Exception as e:
        body = error_container.format(msg=e)
    finally:
        if os.path.exists(mi_log_path):
            os.remove(mi_log_path)
        content = head.format(body=body)
        print(content)
