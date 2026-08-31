# RV1106 오토포커스 도구
#
# 보드용 (SDK 없이, 우분투 기본 크로스컴파일러 + 정적 링크):
#   sudo apt install gcc-arm-linux-gnueabihf
#   make board
#
# 보드용 (Luckfox SDK 툴체인이 있는 경우):
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

.PHONY: all board test clean install

all: $(TARGET)

# 보드 rootfs 는 uClibc 라 glibc 동적 링크 바이너리가 안 돈다.
# 정적으로 링크하면 libc 를 통째로 넣으므로 어느 rootfs 에서도 실행된다.
# 용량은 500KB 쯤 되지만 이 용도에는 문제되지 않는다.
board:
	$(MAKE) clean
	$(MAKE) CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
	@echo
	@echo "빌드 완료. 보드로 복사하세요:"
	@echo "  adb push af_tool /root/  &&  adb shell chmod +x /root/af_tool"

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
