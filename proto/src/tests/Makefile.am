test_files = \
	assoc.test \
	compiler.test \
	complex.test \
	math.test \
	mathlib.test \
	position.test \
	smoke.test \
	tuple.test \
	vectcomp.test

bin_SCRIPTS = prototest.py

# installed tests

installcheck-local:
	$(PYTHON) $(srcdir)/prototest.py \
		--proto=$(bindir)/proto \
		--p2b=$(bindir)/p2b \
		--demos=$(top_srcdir)/demos \
		`for t in $(test_files); do echo $(srcdir)/$$t; done`
	rm -rf dumps

# source-dir checks

check-local:
	$(PYTHON) $(srcdir)/prototest.py \
		--proto=$(top_builddir)/proto \
		--p2b=$(top_builddir)/p2b \
		--demos=$(top_srcdir)/demos \
		`for t in $(test_files); do echo $(srcdir)/$$t; done`
	rm -rf dumps

# cleanup

clean-local:
	rm -rf dumps
	rm -rf *.RESULTS

