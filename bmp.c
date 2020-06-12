#include <stdio.h> // 표준 입출력 헤더
#include <stdlib.h> // malloc 함수를 사용하기 위한 헤더
#include <stdint.h> // uint16_t, uint32_t 등 자료형을 사용하기 위한 헤더
#include <time.h> // 랜덤변수 생성

typedef struct bitmap_header { // 비트맵 헤더 (14바이트)
    uint16_t bfType;           // BMP 파일 매직 넘버
    uint32_t bfSize;           // 파일 크기
    uint32_t bfReserved;        // 예약 공간
    uint32_t bfOffBits;           // 데이터 시작 위치
} bitmap_header;

typedef struct bitmap_info_header { // 비트맵 정보 헤더 (40바이트)
    uint32_t biSize;           // 현재 구조체의 크기
    int32_t biWidth;          // 비트맵 이미지의 가로 크기
    int32_t biHeight;         // 비트맵 이미지의 세로 크기
    uint16_t biPlanes;         // 사용하는 색상판의 수
    uint16_t biBitCount;       // 픽셀 하나를 표현하는 비트 수
    uint32_t biCompression;    // 압축 방식
    uint32_t biSizeImage;      // 비트맵 이미지의 픽셀 데이터 크기
    int32_t biXPelsPerMeter;  // 그림의 가로 해상도(미터당 픽셀)
    int32_t biYPelsPerMeter;  // 그림의 세로 해상도(미터당 픽셀)
    uint32_t biClrUsed;        // 색상 테이블에서 실제 사용되는 색상 수
    uint32_t biClrImportant;   // 비트맵을 표현하기 위해 필요한 색상 인덱스 수
} bitmap_info_header;

typedef struct RGB {
    uint8_t b;          // 파랑
    uint8_t g;         // 초록
    uint8_t r;           // 빨강
} RGB;


void pixel_conv(RGB *in, RGB *out, uint8_t s, uint8_t r, uint8_t g, uint8_t b) { // 픽셀값 변형
    uint8_t tmp;
    switch(s) {
        case 1: // 원본 그대로
            out->r = in->r;
            out->g = in->g;
            out->b = in->b;
            break;
        case 2: // RGB 농도 증가
            out->r = ( ( (int16_t)(r + in->r) > 255) ) ? 0xff : (r + in->r); // 0xff를 넘으면 0x00부터 시작이므로 0xff 값을 넘으면 0xff로 고정
            out->g = ( ( (int16_t)(g + in->g) > 255) ) ? 0xff : (g + in->g);
            out->b = ( ( (int16_t)(b + in->b) > 255) ) ? 0xff : (b + in->b);
            break;
        case 3: // RGB 농도 감소
            out->r = ( ( (int16_t)(-r + in->r) < 0) ) ? 0x00 : in->r - r; // 위 증가와 비슷함
            out->g = ( ( (int16_t)(-g + in->g) < 0) ) ? 0x00 : in->g - g;
            out->b = ( ( (int16_t)(-b + in->b) < 0) ) ? 0x00 : in->b - b;
            break;
        case 4: // 흑백
            tmp = (uint8_t) (in->r * 0.2989 + in->g * 0.5870 + in->b * 0.1140); // 흑백 변환에 사용되는 알고리즘 사용
            out->r = tmp;
            out->g = tmp;
            out->b = tmp;
            break;
        case 5: // 반전
            out->r = ~in->r; // 픽셀 색상 반전
            out->g = ~in->g;
            out->b = ~in->b;
            break;
        case 6: // 한계값
            out->r = in->r < r ? 0x00 : 0xff; // 특정 레벨 이상/이하로 끌어올리거나 끌어내림
            out->g = in->g < g ? 0x00 : 0xff;
            out->b = in->b < b ? 0x00 : 0xff;
            break;
    }
}


int main(void) {
    FILE *fp_origin = NULL, *fp_target = NULL; // 파일 포인터 선언 후 NULL로 초기화
    char origin_filename[60], target_filename[60]; // 파일 이름을 저장하기 위한 문자열 배열
    uint8_t padding_zero[10] = {0, }; // 패딩 비트 삽입을 위한 문자열
    int command; // 키보드로 입력한 명령이 여기 저장됨
    RGB targetrgb, *originrgb; // RGB 제어를 위한 구조체 선언
    bitmap_header h; // 비트맵 파일 헤더
    bitmap_info_header info; // 비트맵 파일 헤더 (상세정보)
    h.bfType = 0; // 사진 파일을 불러오면 값이 대입됨
    int padding, width, height; // 패딩, 넓이, 높이
    int i, j; // 루프문을 위한
    int adj, adj_r, adj_g, adj_b; // RGB 제어를 할때 제어량
    srand(time(NULL)); // 난수 생성을 위한

    printf("================ bmp 포토 이펙터 ================\n");

    menu: {
        if(!h.bfType) {
            printf("사진 파일을 불러와 주세요  (파일명을 입력하면 불러올 수 있습니다.)\n");
            scanf("%s", origin_filename);
            while(getchar() != '\n');
            fp_origin = fopen(origin_filename, "r+");
            if(fp_origin == NULL) {
                printf("파일 불러오기에 실패하였습니다.\n");
                goto menu;
            }
            fread(&h.bfType, sizeof(h.bfType), 1, fp_origin);
            fread(&h.bfSize, sizeof(h.bfSize), 1, fp_origin);
            fread(&h.bfReserved, sizeof(h.bfReserved), 1, fp_origin);
            fread(&h.bfOffBits, sizeof(h.bfOffBits), 1, fp_origin);
          
            fread(&info.biSize, sizeof(info.biSize), 1, fp_origin);
            fread(&info.biWidth, sizeof(info.biWidth), 1, fp_origin);
            fread(&info.biHeight, sizeof(info.biHeight), 1, fp_origin);
            fread(&info.biPlanes, sizeof(info.biPlanes), 1, fp_origin);
            fread(&info.biBitCount, sizeof(info.biBitCount), 1, fp_origin);
            fread(&info.biCompression, sizeof(info.biCompression), 1, fp_origin);
            fread(&info.biSizeImage, sizeof(info.biSizeImage), 1, fp_origin);
            fread(&info.biXPelsPerMeter, sizeof(info.biXPelsPerMeter), 1, fp_origin);
            fread(&info.biYPelsPerMeter, sizeof(info.biYPelsPerMeter), 1, fp_origin);
            fread(&info.biClrUsed, sizeof(info.biClrUsed), 1, fp_origin);
            fread(&info.biClrImportant, sizeof(info.biClrImportant), 1, fp_origin);

            padding = (4 - ((info.biWidth * (info.biBitCount/8)) % 4)) % 4; // 4바이트 단위로 픽셀 처리하므로 패딩크기 구함
            width = (int) info.biWidth;
            height = (int) info.biHeight;
            originrgb = (RGB*) malloc(sizeof(RGB) * width * height);
            if(originrgb == NULL){
                printf("픽셀 정보 저장을 위한 메모리 할당 실패.. 프로그램을 종료합니다\n");
                return -1;
            }
            for (j = 0; j < height; j++) {
                for (i = 0; i < width; i++) {
                    fread((uint8_t*) &originrgb[(j * width) + i], 1, 3, fp_origin); // 동적 할당한 메모리에 픽셀정보 저장
                    if(i == (width - 1)) {
                        fseek(fp_origin, padding, SEEK_CUR); // 패딩비트 건너뛰기
                    }
                }
            }
            printf("파일 읽기 완료..%d x %d, %dbyte\n", info.biWidth, info.biHeight, h.bfSize);
            fclose(fp_origin);
            goto menu;
        } else {
            printf("================ 실행 가능한 기능 ================\n");
            printf("1. 원본파일 복사\t\t2. 밝기 증가\n");
            printf("3. 밝기 감소\t\t\t4.흑백으로 바꾸기\n");
            printf("5. 반전\t\t\t\t6.한계값\n");
            printf("7. 노이즈 추가\t\t\t8.특정 색감 증가\n");
            printf("9. 특정 색감 감소\t\t10.종료\n");
            printf("실행할 기능 번호를 입력하세요 : ");
            scanf("%d", &command);
            if(command > 10) {
                printf("실행 불가능한 기능입니다.\n");
                goto menu;
            }
            if(command == 10) {
                free(originrgb);
                printf("프로그램 종료\n");
                return 0;
            }
            goto targetfile;
        }
    }

    targetfile: {
        if(fp_target != NULL)
            fclose(fp_target);
        printf("저장할 파일명을 입력하세요\n");
        scanf("%s", target_filename);
        while(getchar() != '\n');
        fp_target = fopen(target_filename, "w+");
        if(fp_target == NULL) {
            printf("파일 저장 오류입니다.\n");
            goto targetfile;
        }
        fwrite(&h.bfType, sizeof(h.bfType), 1, fp_target);
        fwrite(&h.bfSize, sizeof(h.bfSize), 1, fp_target);
        fwrite(&h.bfReserved, sizeof(h.bfReserved), 1, fp_target);
        fwrite(&h.bfOffBits, sizeof(h.bfOffBits), 1, fp_target);
      
        fwrite(&info.biSize, sizeof(info.biSize), 1, fp_target);
        fwrite(&info.biWidth, sizeof(info.biWidth), 1, fp_target);
        fwrite(&info.biHeight, sizeof(info.biHeight), 1, fp_target);
        fwrite(&info.biPlanes, sizeof(info.biPlanes), 1, fp_target);
        fwrite(&info.biBitCount, sizeof(info.biBitCount), 1, fp_target);
        fwrite(&info.biCompression, sizeof(info.biCompression), 1, fp_target);
        fwrite(&info.biSizeImage, sizeof(info.biSizeImage), 1, fp_target);
        fwrite(&info.biXPelsPerMeter, sizeof(info.biXPelsPerMeter), 1, fp_target);
        fwrite(&info.biYPelsPerMeter, sizeof(info.biYPelsPerMeter), 1, fp_target);
        fwrite(&info.biClrUsed, sizeof(info.biClrUsed), 1, fp_target);
        fwrite(&info.biClrImportant, sizeof(info.biClrImportant), 1, fp_target);
        goto execute;
    }

    execute: {
        if((command == 2) || (command == 3) ) {
            printf("밝기 %s량을 입력하세요 (0~255)\n", (command == 2) ? "증가" : "감소");
            scanf("%d", &adj);
            while(getchar() != '\n');
        } else if(command == 6) {
            printf("한계값을 입력하세요 (0~255)\n");
            scanf("%d", &adj);
            while(getchar() != '\n');
        } else if(command == 7) {
            printf("노이즈 양을 입력하세요 (0~255)\n");
            scanf("%d", &adj);
            while(getchar() != '\n');
        }

        adj_r = adj;
        adj_g = adj;
        adj_b = adj;

        if(command == 8) {
            command = 2;
            printf("증가량을 입력하세요 (R, G, B 순으로)");
            scanf("%d %d %d", &adj_r, &adj_g, &adj_b);
            while(getchar() != '\n');
            printf("입력된 숫자 : %d %d %d\n", adj_r, adj_g, adj_b);

        } else if(command == 9) {
            command = 3;
            printf("감소량을 입력하세요 (R, G, B 순으로)");
            scanf("%d %d %d", &adj_r, &adj_g, &adj_b);
            while(getchar() != '\n');
            printf("입력된 숫자 : %d %d %d\n", adj_r, adj_g, adj_b);
        }
        for (j = 0; j < height; ++j) {
            for (i = 0; i < width; ++i) {
                if(command == 7) {
                    pixel_conv(&originrgb[(j * width) + i], &targetrgb, 2, (uint8_t) (rand() % adj_r), (uint8_t) (rand() % adj_g), (uint8_t) (rand() % adj_b));
                } else {
                    pixel_conv(&originrgb[(j * width) + i], &targetrgb, command, (uint8_t) adj_r, (uint8_t) adj_g, (uint8_t) adj_b);
                }
                fwrite((uint8_t*) &targetrgb, 1, 3, fp_target);
                if(i == (width - 1)) {
                    fwrite(&padding_zero, 1, padding, fp_target);
                }
            }
        }
        fclose(fp_target);
        printf("성공적으로 저장하였습니다.\n");
        goto menu;
    }
}