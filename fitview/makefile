CC=gcc
CFLAGS=-I"." -I"include/" 

# FIT_USE_STDINT_H can also be defined in fit_config.h
ifneq ($(OS),Windows_NT)
	CFLAGS+=-DFIT_USE_STDINT_H
endif

fitview: fitview.o fit.o fit_convert.o fit_crc.o fit_product.o
	$(CC) $(CFLAGS) fitview.o fit.o fit_convert.o fit_crc.o fit_product.o -o fitview -lm -lraylib 

fitview.o: fitview.c
	$(CC) $(CFLAGS) -c fitview.c -o fitview.o

fit.o: include/fit.c include/fit_config.h include/fit_include.h
	$(CC) $(CFLAGS) -c include/fit.c -o fit.o

fit_convert.o: include/fit_convert.c include/fit_config.h include/fit_include.h
	$(CC) $(CFLAGS) -c include/fit_convert.c -o fit_convert.o

fit_crc.o: include/fit_crc.c include/fit_config.h include/fit_include.h
	$(CC) $(CFLAGS) -c include/fit_crc.c -o fit_crc.o

fit_product.o: include/fit_product.c include/fit_config.h include/fit_include.h
	$(CC) $(CFLAGS) -c include/fit_product.c -o fit_product.o

clean:
	rm -f fitview *.o

