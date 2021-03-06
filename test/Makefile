.PHONY: verify-python-path

verify-python-path:
ifndef VPP_PYTHON_PREFIX
	$(error VPP_PYTHON_PREFIX is not set)
endif

PYTHON_VENV_PATH=$(VPP_PYTHON_PREFIX)/virtualenv

test: wipe verify-python-path
	@virtualenv $(PYTHON_VENV_PATH)
	@bash -c "source $(PYTHON_VENV_PATH)/bin/activate && pip install scapy"
	@bash -c "source $(PYTHON_VENV_PATH)/bin/activate && pip install pexpect"
	@bash -c "source $(PYTHON_VENV_PATH)/bin/activate && cd $(WS_ROOT)/vpp-api/python && python setup.py install"
	@bash -c "source $(PYTHON_VENV_PATH)/bin/activate && python run_tests.py discover -p test_$(TEST)\"*.py\""

retest: wipe verify-python-path
	@bash -c "source $(PYTHON_VENV_PATH)/bin/activate && python run_tests.py discover -p test_$(TEST)\"*.py\""

.PHONY: wipe doc

wipe: verify-python-path
	@rm -f /dev/shm/vpp-unittest-*
	@rm -rf /tmp/vpp-unittest-*

doc: verify-python-path
	@virtualenv $(PYTHON_VENV_PATH)
	@bash -c "source $(PYTHON_VENV_PATH)/bin/activate && pip install sphinx"
	@bash -c "source $(PYTHON_VENV_PATH)/bin/activate && make -C doc WS_ROOT=$(WS_ROOT) BR=$(BR) NO_VPP_PAPI=1 html"

wipe-doc:
	@make -C doc wipe BR=$(BR)

help:
	@echo "Running tests:"
	@echo ""
	@echo " test                - build and run functional tests"
	@echo " test-debug          - build and run functional tests (debug build)"
	@echo " retest              - run functional tests"
	@echo " retest-debug        - run functional tests (debug build)"
	@echo " wipe-test           - wipe (temporary) files generated by unit tests"
	@echo ""
	@echo "Arguments controlling test runs:"
	@echo " V=[0|1|2]            - set test verbosity level"
	@echo " DEBUG=<type>         - set VPP debugging kind"
	@echo "    DEBUG=core        - detect coredump and load it in gdb on crash"
	@echo "    DEBUG=gdb         - allow easy debugging by printing VPP PID "
	@echo "                        and waiting for user input before running "
	@echo "                        and tearing down a testcase"
	@echo "    DEBUG=gdbserver   - run gdb inside a gdb server, otherwise "
	@echo "                        same as above"
	@echo " STEP=[yes|no]        - ease debugging by stepping through a testcase "
	@echo " TEST=<name>          - only run specific test"
	@echo ""
	@echo "Creating test documentation"
	@echo " test-doc            - generate documentation for test framework"
	@echo " wipe-test-doc       - wipe documentation for test framework"
	@echo ""
