pdf:
	@pdflatex main
	@echo "Building done. ;)"

clean:
	@rm -rf main.aux main.bbl main.blg main.log main.nav main.out main.run.xml main.snm main.toc bu.aux

distclean: clean
	@rm -rf main.pdf

rebuild: clean all
