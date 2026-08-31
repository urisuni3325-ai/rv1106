# RV1106 오토포커스 도구
#
# 보드용 (크로스컴파일):
#   make CROSS_COMPILE=arm-rockchip830-linux-uclibcgnueabihf-
#
# 호스트 테스트 (보드 없이):
#   make test

CROSS_COMPILE ?=
CC      := $(CROSS_COMPILE)gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c99 -D_GNU_SOURCE
LDFLAGS ?=

TARGET  := af_tool
OBJS    := af_tool.o gpio_i2c.o dw9714.o focus_score.o af_search.o v4l2_cap.o

TEST_BIN  := test_af
TEST_OBJS := test_af.o af_search.o focus_score.o

.PHONY: all test clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 테스트는 항상 호스트 컴파일러로 빌드한다 (크로스 설정과 무관하게 돌아야 한다).
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_OBJS:.o=.c)
	cc $(CFLAGS) -o $@ test_af.c af_search.c focus_score.c -lm

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

af_tool.o:     af_search.h dw9714.h focus_score.h gpio_i2c.h v4l2_cap.h
gpio_i2c.o:    gpio_i2c.h
dw9714.o:      dw9714.h gpio_i2c.h
focus_score.o: focus_score.h
af_search.o:   af_search.h
v4l2_cap.o:    v4l2_cap.h

install: $(TARGET)
	@echo "보드로 복사:  adb push $(TARGET) /root/"

clean:
	rm -f $(TARGET) $(TEST_BIN) *.o
