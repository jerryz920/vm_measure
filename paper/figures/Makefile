all:
	for f in $(ls *.ps); do ps2pdf -dPDFSETTINGS=/prepress $f; done
	for f in $(ls *.eps); do epstopdf $f; done
