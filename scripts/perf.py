#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved.
"""TE Performance JSON processing and plotting/tabling"""

import argparse
import copy
import fileinput
import json
import pprint
import sys
import re
from itertools import chain
from typing import Any, Dict, List, Optional, Tuple, Set, Union

import matplotlib.pyplot as plt
from pytablewriter import ExcelXlsxTableWriter, MarkdownTableWriter

pp = pprint.PrettyPrinter(indent=4)


def plot_one(
    reports: List[Dict[str, Dict[str, str]]],
    name: str,
    x_axis: Optional[str],
    mfilter: Optional[Dict[str, str]] = None,
    debug: bool = False
) -> Tuple[List[str], List[List[str]], List[int], List[float]]:
    '''Add a single plot (or table) into the report

    The output depends on the specified format

    Args:
        reports: List of reports collected from JSON
        name: Name of the measurement (as per JSON!)
        x_axis: Name of the key to be used as X axis (or first column)
        mfilter (dict, optional): Filter to be used to pick results for the plot, measurement
                                  key:value where key is the measurement key. Defaults to {}.
        debug (bool, optional): Provide debug logging (will break the syntax!). Defaults to False.

    Raises:
        Exception: Different measurements have different list of keys - forms
                   invalid/incorrect MI

    Returns:
    '''
    if mfilter:
        for key, value in mfilter.items():
            reports = [
                r for r in reports if r['keys'][key].lower() == value.lower()
            ]

    x_points = []
    y_points = []

    def natural_sort_key(value: str) -> List[Union[int, str]]:
        '''Converts value into human-sortable value

        Args:
            value: Item in a sortable object

        Returns:
            The input value split into human-sortable elements
        '''
        return [
            int(s) if s.isdecimal() else s for s in re.split(r'(\d+)', value)
        ]

    if x_axis:
        reports = sorted(reports,
                         key=lambda x: natural_sort_key(x['keys'][x_axis]))
    if debug:
        for report in reports:
            pp.pprint(report)

    report_get_point.count = 0
    table = []
    top_headers: List[str] = []
    for report in reports:
        x_point, y_point, headers = report_get_point(report, name, x_axis,
                                                     mfilter)
        if not top_headers:
            top_headers = headers
        elif top_headers != headers:
            raise Exception(
                f'You are trying to unite {headers} and {top_headers} into one table'
            )
        x_points.append(int(x_point))
        y_points.append(float(y_point))
        table.append([x_point] + [y_point] + list(report['keys'].values()))

    if len(x_points) == 0:
        if debug:
            # We don't raise an error - no resuls means no graph, but it does not
            # mean there is a problem, cause may be we have a list of standard plots specified
            # in the command line and log has only subset of results.
            print('No results found')

    return top_headers, table, x_points, y_points


# I don't know how to describe what 'options' is in type hinting
def parse_options() -> argparse.Namespace:
    '''Parse command line options

    Returns:
        Options structure and list of unparsed strings in argv, same as Parser.parse_args()
    '''
    parser = argparse.ArgumentParser(
        usage='''perf.py [options] X-Axis [Key=Value[/Key=Value]*]+

    Examples:

    # All measurements in chronological order; first column will be measurement
    # number and second will be the measurement itself. Then there are run parameters.

    ../virtblk-ts/scripts/perf.py -i 'mi.txt' -c -m 'Read iops'

    # Let's sort by Dev1-R-Threads

    ../virtblk-ts/scripts/perf.py -i 'mi.txt' -c -m 'Read iops' Dev1-R-Threads

    # Add more filtering to narrow down the table

    ../virtblk-ts/scripts/perf.py -i 'mi.txt' -c -m 'Read iops' Dev1-R-Threads Dev=1/FIO-iodepth=64/FIO-numjobs=4/FIO-rwmixread=100/FIO-blocksize={4096,1048576}

    # You can read MI from standard input:

    ../te/log.sh --mi | ../virtblk-ts/scripts/perf.py  -c -m 'Read iops'

    Note, that you might want to install dependencies:

      $ pip3 install -r requirements.txt # requirements file located next to the script
                                         # depending on the distro you might need extra options
      $ apt install gnuplot              # used to graph things
                     ''')
    parser.add_argument(
        '-i',
        '--input',
        dest='input',
        default=sys.stdin,
        help=
        'File to read MI artifacts from. By default data is read from stdin',
        metavar='FILE')
    parser.add_argument('-o',
                        '--output',
                        dest='output_file',
                        default='graph.png',
                        help='File to write output to',
                        metavar='FILE')
    parser.add_argument('-m',
                        '--measurement',
                        dest='measurement',
                        help='Measurement name (Read iops/Write iops/etc.')
    parser.add_argument('-d',
                        '--debug',
                        action='store_true',
                        dest='debug',
                        default=False,
                        help='Enable debug logging')
    parser.add_argument('-c',
                        '--console',
                        action='store_true',
                        dest='console',
                        default=False,
                        help='Log tables in console')
    parser.add_argument('-e',
                        '--excel',
                        action='store_true',
                        default=False,
                        help='Dump Excel table into the specified file')
    parser.add_argument('-g',
                        '--gnuplot',
                        action='store_true',
                        default=False,
                        help='Dump Gnuplot into the specified file')
    parser.add_argument('--gnuplot-y-log',
                        action='store_true',
                        dest='y_log',
                        default=False,
                        help='Use logarithmic scale for Y axis')
    parser.add_argument('rest', nargs='*')
    try:
        options = parser.parse_args()
    except argparse.ArgumentError as error:
        print(f'Failed to parse arguments: {error}')
        sys.exit(1)

    return options


def report_get_point(
        report: Dict, name: str, x_axis: Optional[str],
        mfilter: Optional[Dict[str, str]]) -> Tuple[str, str, List[str]]:
    '''Get data point from a single performance report

    Args:
        report: Report dict, format as from JSON
        name: Name of the measurement to get
        x_axis: Name of the key value for which will be used as X axis
        mfilter: Measurement keys:value pairs as in JSON, used to select
                 results from the report

    Raises:
        Exception: Found a measurement without values
        Exception: Found measurement with more than one value

    Returns:
        Value of the key (x_axis), measured value
    '''
    report_get_point.count += 1
    # We take the first measurement with the specified name, if more measurements
    # exist - sorry.
    results = [r for r in report['results'] if r['name'] == name]
    if len(results) != 1:
        raise Exception(f'Expecting exactly one result for {name} '
                        '({mfilter.keys()}), not sure what to do: {results}')

    values_list = results[0]['entries']
    if not values_list:
        raise Exception(
            'Measurement result without actual values, not sure what you mean')
    if len(values_list) > 1:
        raise Exception('Measurement result with >1 value, not supported')

    value = values_list[0]

    # We remove some bits of the key since we know that it's the same
    report['keys'].pop('FIO-cmd', None)
    if mfilter:
        for key in mfilter.keys():
            report['keys'].pop(key, None)

    if x_axis:
        first_column_name = x_axis
        first_column_value = report['keys'][x_axis]
    else:
        first_column_name = "Measurement#"
        first_column_value = str(report_get_point.count)

    headers = [first_column_name] + [
        name +
        f" ({value['base_units']} * {float.fromhex(value['multiplier'])})"
    ] + list(report['keys'].keys())

    return first_column_value, value['value'], headers


def md_write(name: str, headers: List[str], table: List[List[Any]]) -> None:
    '''Write a Markdown table into stdout

    Args:
        name: Name of the table
        headers: Headers of the table
        table: Two dimentional table
    '''
    writer = MarkdownTableWriter(table_name=name,
                                 headers=headers,
                                 value_matrix=table)
    writer.write_table()


def excel_write(name: str, headers: List[str], table: List[List[Any]],
                output: str) -> None:
    '''Write an Excel table into specified file path

    Args:
        name: Name of the table
        headers: Headers of the table
        table: Two dimentional table
    '''
    writer = ExcelXlsxTableWriter(table_name=name,
                                  headers=headers,
                                  value_matrix=table)
    writer.dump(output=output)


def gnuplot_add(figure_name: str, name: str, x_axis: str, y_axis: str,
                x_points: List[int], y_points: List[float],
                y_log: bool) -> None:
    '''Add a single plot to the gnuplot figure that will be generated at the end

    Args:
        figure_name: Name of the figure
        name: Name of the plot
        x_axis: X axix legend
        y_axis: Y axis legend
        x_points: List of points on the X
        y_points: List of points on the Y
    '''
    plt.title(figure_name)
    plt.xlabel(x_axis)
    plt.ylabel(y_axis)
    plt.grid()
    if y_log:
        plt.yscale('log')
    for x_point, y_point in zip(x_points, y_points):
        if isinstance(y_points, int):
            plt.annotate(f'{y_point:.5d}',
                         xy=(x_point, y_point),
                         color='dodgerblue')
        elif isinstance(y_point, float):
            plt.annotate(f'{y_point:.3f}',
                         xy=(x_point, y_point),
                         color='dodgerblue')
        else:
            raise Exception(
                f'Can handle point {y_point} of type {y_point.__class__}')
    plt.scatter(x_points, y_points)
    plt.plot(x_points, y_points, '-', label=name)


def read_input(json_source: Any) -> List[str]:
    '''Return list of lines from the specified input

    Args:
        input: sys.stdout or a file

    Returns:
        Array of strings read from the input
    '''
    if json_source is sys.stdin:
        input_files = []
    else:
        input_files = [json_source]
    lines = []
    try:
        with fileinput.input(files=input_files) as file:
            for line in file:
                lines.append(line.rstrip())
    except FileNotFoundError as e:
        print(f'Failed to read input from {input}: {e}\n\n', file=sys.stderr)
        sys.exit(1)

    return lines


def parse_plotfilter(arg: str) -> Dict[str, str]:
    '''Parse plot filter definition arguments

    Args:
        arg: JSON key=value/key=value string

    Returns:
        Dictiionary with measurement keys (named as in JSON) and values specified in
        command line
    '''

    if not '=' in arg:
        return {}

    # This is required since mypy does not get that maxsplit=1 results into at most a tuple!
    kv_list = [arg.split('=', maxsplit=1) for arg in arg.split('/')]
    for kv_spec in kv_list:
        if len(kv_spec) != 2:
            print(
                f'You passed {kv_spec} as part of the plot specifier, but format is KEY=VALUE'
            )
            sys.exit(1)

    return {kv[0]: kv[1] for kv in kv_list}


def main() -> int:
    """Main function

    Encapsulated to avoid global visibility of variables

    Returns:
        int: Return code to be passed to system (0 - ok)
    """
    options = parse_options()
    # What remains is ... <x axis key name> key=value/key=value...
    if options.rest:
        x_axis = options.rest[0]
    else:
        x_axis = None

    lines = read_input(options.input)
    reports = [json.loads(line) for line in lines]
    # People insert random garbage, this line should be removed once we find out whom!
    reports = [x for x in reports if isinstance(x, dict)]
    perf_reports = [x for x in reports if x.get('type') == 'measurement']

    if options.debug:
        print('Generating figures:')
        print(f'  input: {options.input}')
        print(f'  output: {options.output_file}')
        for report in perf_reports:
            pp.pprint(report)
        print(f'  x_asix: {x_axis}')

    if options.gnuplot:
        fig = plt.figure(figsize=(16, 12), dpi=80)

    plot_filters_strings = options.rest[1:]
    if not plot_filters_strings:
        plot_filters_strings = [' ']

    def compare_plotfilters(index: int) -> Tuple[Set[str], Set[str]]:
        '''Return common and unique plot filter items

        Args:
            index: position in plot_filters_strings list to compare with

        Returns:
            Subsets of plot_filters_strings
        '''
        split_pfs = [set(pf.split('/')) for pf in plot_filters_strings]
        common = split_pfs[0].intersection(
            *split_pfs[1 if len(split_pfs) > 0 else 0:])
        unique = split_pfs[index].difference(common)
        return common, unique

    for index, arg in enumerate(plot_filters_strings):
        plot_common, plot_unique = compare_plotfilters(index)
        plot_filter = parse_plotfilter(arg)
        plot_label = f'{", ".join(sorted(plot_unique, reverse=True))}'
        figure_label = f'{options.measurement}: {", ".join(sorted(plot_common, reverse=True))}'

        headers, table, xs_list, ys_list = plot_one(
            reports=copy.deepcopy(perf_reports),
            name=options.measurement,
            x_axis=x_axis,
            mfilter=plot_filter,
            debug=options.debug)

        if options.console:
            md_write(figure_label, headers, table)
        if options.excel:
            excel_write(figure_label, headers, table, options.output_file)
        if options.gnuplot:
            if not x_axis:
                print('You need to specify X axis to plot')
                return 1
            gnuplot_add(figure_label,
                        plot_label,
                        x_axis=x_axis,
                        y_axis=options.measurement,
                        x_points=xs_list,
                        y_points=ys_list,
                        y_log=options.y_log)

    if options.gnuplot:
        fig.show()
        fig.legend()
        fig.savefig(options.output_file)

    return 0


if __name__ == '__main__':
    sys.exit(main())
