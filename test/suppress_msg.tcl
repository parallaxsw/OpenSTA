# suppress and unsuppress message ids

set sta_continue_on_error 1

sta::report_warn 1234 "EXPECTED: This is a warning message"
sta::report_error 5678 "EXPECTED: This is an error message"

suppress_msg {1234 5678}

sta::report_warn 1234 "UNEXPECTED: This is a warning message"
sta::report_error 5678 "UNEXPECTED: This is an error message"

unsuppress_msg {1234 5678}

sta::report_warn 1234 "EXPECTED: This is a warning message"
sta::report_error 5678 "EXPECTED: This is an error message"
