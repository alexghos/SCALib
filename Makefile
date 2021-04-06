
.PHONY: dev test devtest coverage docs


dev:
	tox -e dev
	@echo "Run source .tox/dev/bin/activate to activate development virtualenv."

test:
	tox -e test

devtest:
	tox -e dev pytest

coverage:
	tox -e test-cov
	@echo "Open htmlcov/index.html to see detailled coverage information."

docs:
	tox -e build_docs