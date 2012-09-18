all: clean
	xelatex cv
	bibtex cv
	xelatex cv
	xelatex cv
	pdflatex position
	pdflatex report

pdf: all

clean:
	@rm -rf *.aux *.log *.out *.pdf *.log *.bbl *.blg
