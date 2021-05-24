#[作りたいもの]: [材料]
#\tab			[作り方]
CFLAGS=-std=c11 -g -static	#C compiler option

teacc: teacc.c

test: teacc
		./test.sh

clean:
		rm -f teacc *.o *~ tmp*

.PHONY: test clean			#cleanではファイルを生成しない(cleanというファイルができてしまうとcleanできなくなる)