#[作りたいもの]: [材料]
#\tab			[作り方]

###変数
CFLAGS=-std=c11 -g -static	#c11の記述であること / デバッグ情報を出力すること / staticリンクすることを指定
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
###

teacc: teacc.c
		$(CC) -o teacc $(OBJS) $(LDFLAGS)

$(OBJS): teacc.h

test: teacc
		./test.sh

clean:
		rm -f teacc *.o *~ tmp*

.PHONY: test clean			#cleanではファイルを生成しない(cleanというファイルができてしまうとcleanできなくなる)
