.DEFAULT_GOAL := compile-run
.RECIPEPREFIX := > 

INPUT = "1 + 2"

compile:
> gcc eval.c -Wall -Wextra -lm -std=c2x -o eval

compile-run: compile
> ./eval "$(INPUT)"

run:
> ./eval "$(INPUT)"