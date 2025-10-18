# Convenience makefile for building and running pd_multi_test
.PHONY: pd-multi-test

pd-multi-test:
	mkdir -p build
	cd build && cmake -DENABLE_LIBPD_MULTI=ON .. && cmake --build . --target pd_multi_test -j$(shell nproc)
	./build/src/tools/pd_multi_test
