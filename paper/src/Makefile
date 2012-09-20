# GNU/Linux Makefile for the cloud paper

pdf:
	pdflatex main
	bibtex main
	pdflatex main
	pdflatex main

clean:
	@rm -rf main.aux main.bbl main.blg *.log *.out

clobber: clean
	@rm -rf main.pdf ../figures/*-eps-converted-to.pdf

help:
	@echo "God helps those who help themselves."
