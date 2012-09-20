epsfiles = $(wildcard *.eps)
psfiles = $(wildcard *.ps)
all:
	for f in $(psfiles); do ps2pdf -dPDFSETTINGS=/prepress $$f; done
	for f in $(epsfiles); do echo $$f; epstopdf --nocompress $$f; done
