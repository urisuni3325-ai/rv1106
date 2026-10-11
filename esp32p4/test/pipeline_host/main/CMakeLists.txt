# pipeline.c 를 그대로 가져와서, 하드웨어 함수(디코드/인코드/전송)만 test_main.c 의 가짜로 바꿔 돌린다
idf_component_register(SRCS "test_main.c" "../../../main/pipeline.c"
                       INCLUDE_DIRS "." "../../../main"
                       REQUIRES freertos esp_timer log)
