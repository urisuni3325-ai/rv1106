# 호스트 테스트 (보드·ESP-IDF 없이)
#   make -C esp32p4/test         C 유틸 단위 테스트 (gcc, python3 + Pillow)
#   make -C esp32p4/test web     웹 뷰어 브라우저 테스트 (node + playwright)
#   make -C esp32p4/test pipeline 영상 파이프라인 동시성 테스트 (ESP-IDF 환경, linux 타깃)

CC      ?= gcc
CFLAGS  ?= -O1 -g -Wall -Wextra -Werror -std=c99 -fsanitize=address,undefined
SRC_DIR := ../main
OUT     := build

.PHONY: test web pipeline clean

test: $(OUT)/test_util
	python3 gen_jpeg.py gen $(OUT)
	$(OUT)/test_util $(OUT)/src_420.jpg $(OUT)/src_422.jpg $(OUT)
	python3 gen_jpeg.py check $(OUT)

web: test
	node web_e2e.mjs ../main/web/index.html $(OUT)/fixed_1280x720.jpg

pipeline:
	cd pipeline_host && idf.py --preview set-target linux && idf.py build && ./build/pipe_test.elf

$(OUT)/test_util: test_util.c $(SRC_DIR)/jpeg_util.c $(SRC_DIR)/sdp_util.c | $(OUT)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $@ $^

$(OUT):
	mkdir -p $@

clean:
	rm -rf $(OUT) pipeline_host/build pipeline_host/sdkconfig
