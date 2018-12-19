/*
 * qr_recognize.c
 *
 *  Created on: 2017年12月31日
 *      Author: sky
 */
#include <stdio.h>
#include <string.h>
#include "quirc_internal.h"
#include "qr_recognize.h"
#include "esp_camera.h"
#include "quirc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static char* TAG="QR";
static const char *data_type_str(int dt) {
	switch (dt) {
	case QUIRC_DATA_TYPE_NUMERIC:
		return "NUMERIC";
	case QUIRC_DATA_TYPE_ALPHA:
		return "ALPHA";
	case QUIRC_DATA_TYPE_BYTE:
		return "BYTE";
	case QUIRC_DATA_TYPE_KANJI:
		return "KANJI";
	}

	return "unknown";
}

void dump_cells(const struct quirc_code *code) {
	int u, v;

	printf("    %d cells, corners:", code->size);
	for (u = 0; u < 4; u++)
		printf(" (%d,%d)", code->corners[u].x, code->corners[u].y);
	printf("\n");

	for (v = 0; v < code->size; v++) {
		printf("    ");
		for (u = 0; u < code->size; u++) {
			int p = v * code->size + u;

			if (code->cell_bitmap[p >> 3] & (1 << (p & 7)))
				printf("[]");
			else
				printf("  ");
		}
		printf("\n");
	}
}

void dump_data(const struct quirc_data *data) {
	printf("    Version: %d\n", data->version);
	printf("    ECC level: %c\n", "MLHQ"[data->ecc_level]);
	printf("    Mask: %d\n", data->mask);
	printf("    Data type: %d (%s)\n", data->data_type,
			data_type_str(data->data_type));
	printf("    Length: %d\n", data->payload_len);
	printf("    Payload: %s\n", data->payload);

	if (data->eci)
		printf("    ECI: %d\n", data->eci);
}

static void dump_info(struct quirc *q) {
	int count = quirc_count(q);
	int i;

	printf("%d QR-codes found:\n\n", count);
	for (i = 0; i < count; i++) {
		struct quirc_code code;
		struct quirc_data data;
		quirc_decode_error_t err;

		quirc_extract(q, i, &code);
		err = quirc_decode(&code, &data);

		dump_cells(&code);
		printf("\n");

		if (err) {
			printf("  Decoding FAILED: %s\n", quirc_strerror(err));
		} else {
			printf("  Decoding successful:\n");
			dump_data(&data);
		}

		printf("\n");
	}
}

void qr_recognize(void *pdata) {
//	i++;
	camera_config_t *camera_config =pdata;
	camera_fb_t* fb = esp_camera_fb_get();
	if((pdata==NULL)||(camera_config->frame_size>FRAMESIZE_VGA))
	{
		ESP_LOGI(TAG,"Camera Size err");
		vTaskDelete(NULL) ;
	}
	printf("begin to qr_recoginze\r\n");
	struct quirc *q;
	struct quirc_data qd;
	uint8_t *image;
	q = quirc_new();
	if (!q) {
		printf("can't create quirc object\r\n");
		vTaskDelete(NULL) ;
	}
	printf("begin to quirc_resize\r\n");

	if (quirc_resize(q, fb->width, fb->height) < 0)
	{
		printf("quirc_resize err\r\n");
		quirc_destroy(q);
		vTaskDelete(NULL) ;
	}
	image = quirc_begin(q, NULL, NULL);
	memcpy(image, fb->buf, fb->len);
	quirc_end(q);
	printf("quirc_end\r\n");
	int id_count = quirc_count(q);
	if (id_count == 0) {
		fprintf(stderr, "Error: not a valid qrcode\n");
		quirc_destroy(q);
		esp_camera_fb_return(fb);
		vTaskDelete(NULL) ;
	}

	printf("qr found\n");
	struct quirc_code code;
	quirc_extract(q, 0, &code);
	quirc_decode(&code, &qd);
	dump_info(q);
	quirc_destroy(q);
    esp_camera_fb_return(fb);
//	j++;
	printf("finish recoginize\r\n");
	vTaskDelete(NULL) ;
}

void qr_recognize_(void *pdata) {
	camera_config_t *camera_config =pdata;
	camera_fb_t* fb = NULL;
	uint8_t *image;
	if((pdata==NULL)||(camera_config->frame_size>FRAMESIZE_VGA))
	{
		ESP_LOGI(TAG,"Camera Size err");
		vTaskDelete(NULL) ;
	}
	struct quirc *q;
	struct quirc_data qd;

	// カメラが起動し、バッファにjpegイメージがコピーされる
	fb = esp_camera_fb_get();

	q = quirc_new();
	if (!q) {
		ESP_LOGI(TAG,"can't create quirc object\r\n");
		vTaskDelete(NULL) ;
	}
	if (quirc_resize(q, fb->width, fb->height) < 0)
	{
		ESP_LOGI(TAG,"quirc_resize err\r\n");
		quirc_destroy(q);
		vTaskDelete(NULL) ;
	}
	// unsigned char *pixels;
	printf("%d, %d, %d\n", fb->width, fb->height, fb->len);
	int width = fb->width;
    int height = fb->height;
    // int bpp = 3;       // 入力画像のピクセル辺りのバイト数
	// int n_channel = 1; // grayscale

	// pixelsにはjpgをパースした結果のビットマップがコピーされる
	// req_comp = 1を指定するとグレースケールに変換される。
	// その時のバッファサイズはピクセル数xチャンネル数 640 x 480 x 1バイト
	// pixels = stbi_load_from_memory(fb->buf, fb->len, &width, &height, &bpp, n_channel);
	// pixels = fb->buf;

	image = quirc_begin(q, &width, &height);
	printf("%d, %d, %d\n", fb->width, fb->height, fb->len);
	memcpy(image, fb->buf, fb->len);
    esp_camera_fb_return(fb);

	/*
	uint8_t *ptr = fb->buf;
	int px = 0;
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			px = *ptr > 120 ? 1 : 0;
			printf("%d", px);
			ptr++;
		}
		// break;
		printf("\n");
	}
	*/

/*
	if (pixels == NULL) {
		ESP_LOGI(TAG,"stbi failure: %s\r\n", stbi_failure_reason());
	} else {
		// 幅と高さを指定して初期化する
		// この段階で想定する画像の形式はなに？ -> グレースケール
		image = quirc_begin(q, &width, &height);

		int buf_size = width * height * n_channel;
		memcpy(image, pixels, buf_size);
		stbi_image_free(pixels);
	}
	*/

	// qrイメージの認識が走る
	quirc_end(q);

	ESP_LOGI(TAG,"quirc_end\r\n");
	int id_count = quirc_count(q);
	if (id_count == 0) {
		ESP_LOGI(TAG,"Error: not a valid qrcode\n");
		quirc_destroy(q);
		vTaskDelete(NULL) ;
	}

	struct quirc_code code;
	quirc_extract(q, 0, &code);
	quirc_decode(&code, &qd);
	dump_info(q);
	quirc_destroy(q);
	ESP_LOGI(TAG,"Error: not a valid qrcode\n");
	vTaskDelete(NULL) ;
}
