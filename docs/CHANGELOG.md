# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1] - 25-07-2025
### Fixed
- Some deadlocks were prone to happen whenever the same mutex was trying to be locked too frequently. An internal control mutex has been introduced to manage associated mutex information.
- Fixed memory-allocation-related issues.

## [1.0] - 16-05-2025
### Added
- First version
