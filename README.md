# Block devices Test Suite

Block device Test Suite allows to test block devices and measure their performance. The main goal is to check that a block device correctly handles I/O requests with different scenarious and device interaction with file systems.

TE is used as an Engine that allows to prepare desired environment for every test. This guarantees reproducible test results.

Block TS provides API for work with various tools like:
* FIO
* dbench
* compilebech
* fsmark
* and other ones.

This API allows to run these tools, get their reports, analyze it and check for expected behaviour in different situatiuns.

## License

See license file in the repository
