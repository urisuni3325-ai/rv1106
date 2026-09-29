# 두피 스캐너용 매크로 렌즈 견적 요청서 (RFQ)

> 이 문서는 광학·카메라 모듈 업체에 보내는 견적 요청서 초안입니다.
> 1부는 한국어, 2부는 영문(해외 업체용), 3부는 중문 핵심 사양표(1688/알리바바 업체용)입니다.
> 부록에는 사양을 정한 근거 계산을 넣었습니다. 업체에 보낼 때는 필요 없으면 빼세요.
> `[ ]` 로 표시한 곳은 보내기 전에 채워 넣으세요.

---

# 1부. 견적 요청서 (한국어)

## 1. 개요

| 항목 | 내용 |
|---|---|
| 요청 회사 | [회사명 / 담당자 / 연락처 / 이메일] |
| 제품 | 휴대용 두피 관찰 카메라 (WiFi 무선, 스마트폰 앱 연동) |
| 요청 대상 | 두피 접사용 매크로 광학계 (아래 옵션 1 또는 옵션 2) |
| 용도 | 모공·모발·각질 관찰 및 기록 (의료 진단 목적 아님) |
| 요청 일자 | [YYYY-MM-DD] |
| 회신 희망일 | [YYYY-MM-DD] |

## 2. 대상 카메라

두 가지 카메라에 대해 각각 견적을 요청합니다. 한쪽만 가능하면 가능한 쪽만 회신해 주세요.

| | 카메라 A | 카메라 B |
|---|---|---|
| 센서 | Sony IMX708 (12MP, 4608×2592, 1.4µm) | Sony IMX219 (8MP, 3280×2464, 1.12µm) |
| 센서 유효 크기 | 6.45 × 3.63 mm | 3.67 × 2.76 mm |
| 기존 렌즈 | Raspberry Pi Camera Module 3 **Wide** (EFL 2.75 mm, F2.2, 대각 120°) | IMX219 AF 모듈 (EFL 약 3.0 mm, F2.0 — **업체 확인 필요**) |
| 초점 방식 | VCM 자동초점 (최소 초점거리 약 5 cm) | VCM 자동초점 (최소 초점거리 약 10 cm — **확인 필요**) |
| 인터페이스 | MIPI CSI-2 | MIPI CSI-2 |

## 3. 목표 광학 사양

| 항목 | 목표값 | 비고 |
|---|---|---|
| **작동거리** | **20 mm** | 카메라(렌즈) 앞면 ~ 두피 표면. 광학계 전체가 이 안에 들어가야 함 |
| **촬영 영역(시야)** | **가로 6 mm** (±5%) | 세로는 센서 비율을 따름 (A: 약 3.4 mm, B: 약 4.5 mm) |
| 화면 배율 | 약 11배 | 스마트폰 세로 화면(폭 약 66 mm) 기준 = 66 ÷ 6 |
| 해상력 (피사체 기준) | 중심 ≥ 50 lp/mm (약 10 µm), 주변 70% 영역 ≥ 35 lp/mm | 모발 굵기 50~100 µm 식별 |
| 피사계 심도 | ≥ 0.5 mm (위 해상력을 유지하는 범위) | 두피 굴곡·모발 높이 차 |
| 왜곡 | ≤ 2% | 모발 굵기·밀도 측정용 |
| 주변 광량비 | ≥ 70% | |
| 파장 | 420 ~ 680 nm (백색 LED) | AR 코팅 |
| 초점 보정 | VCM 행정으로 작동거리 ±0.5 mm 이상 보정 | 조립 공차·누르는 힘 차이 흡수 |
| 조명 | 렌즈 주위에 LED 링 배치 공간 필요 | 교차 편광 필름 사용 예정 |

## 4. 요청 옵션

### 옵션 1 — 기존 카메라 앞에 붙이는 보조 렌즈 (Add-on / Close-up lens)

기존 카메라 모듈은 그대로 두고, 앞에 보조 렌즈를 붙여 위 사양을 만드는 방식입니다.

- 기존 카메라의 VCM 자동초점으로 초점을 맞춥니다.
- 당사 계산으로는 작동거리 20 mm 안에서 **보조 렌즈만으로 가로 6 mm를 센서 전체에 채우기는 어렵습니다** (부록 A).
  현실적인 안은 **광학 시야 12~15 mm + 중앙 6 mm 디지털 크롭**입니다.
- 이 경우 아래를 제안해 주세요.
  - 보조 렌즈 사양 (EFL, 지름, 구성 매수, 재질, 비구면 여부)
  - 카메라 ~ 보조 렌즈 간격, 보조 렌즈 ~ 두피 간격
  - 달성 가능한 광학 시야와 중앙 6 mm 영역의 해상력 (MTF)

### 옵션 2 — 매크로 전용 렌즈로 교체한 카메라 모듈 (권장)

센서에 **20 mm 작동거리 전용 매크로 렌즈**를 VCM 위에 올린 완성 모듈입니다.
보조 렌즈 없이 센서 전체 해상도를 6 mm 시야에 씁니다.

| | 카메라 A (IMX708) | 카메라 B (IMX219) |
|---|---|---|
| 필요한 광학 배율 | 약 1.08배 | 약 0.61배 |
| 참고 설계 (단렌즈 근사) | EFL 약 10.4 mm, 센서~렌즈 약 21.5 mm | EFL 약 7.6 mm, 센서~렌즈 약 12.2 mm |
| 센서~두피 전체 길이 (참고) | 약 42 mm | 약 32 mm |

- 렌즈는 M12 / M8 / 전용 경통 모두 가능하며, **VCM(또는 동등한 구동부)으로 초점 조절**이 되어야 합니다.
- VCM 드라이버 IC: **DW9714 또는 호환 IC**, 센서와 같은 I2C 버스 선호.
- 이미 생산 중인 **피부·두피 검사용 매크로 모듈**이 있으면 그 사양으로 제안해 주셔도 됩니다.

## 5. 기구·환경 조건

| 항목 | 조건 |
|---|---|
| 광학계 최대 외경 | [예: Ø 14 mm 이하] (LED 링과 함께 손잡이 폭 약 35 mm 안에 들어가야 함) |
| 두피 접촉부 | 커버 윈도우(유리 또는 사파이어, AR 코팅) 포함 설계 — 두께만큼 초점 위치 보정 |
| 세척 | 알코올 솜으로 닦아도 코팅·재질 손상 없을 것 |
| 사용 온도 | 0 ~ 40 °C |
| 피부 접촉 재질 | 접촉 부위 재질 정보 제공 요청 |

## 6. 회신 요청 사항

1. 제안 광학 설계 요약 (구성, EFL, F값, 작동거리, 시야)
2. MTF 곡선 (피사체 기준 lp/mm), 피사계 심도, 왜곡, 주변 광량비
3. 2D 도면 및 3D 모델 (STEP)
4. 공차 분석 결과 또는 양산 수율 예상
5. 단가: 샘플 / 100개 / 500개 / 1,000개 / 5,000개
6. 설계비(NRE)·금형비, MOQ
7. 샘플 납기 및 양산 납기
8. 유사 제품(피부·두피·치과 구강 카메라 등) 양산 이력

## 7. 수량 계획

| 단계 | 수량 | 시기 |
|---|---|---|
| 샘플 | [5~10] | [ ] |
| 시험 생산 | [100~300] | [ ] |
| 양산 | [1,000 / 년] | [ ] |

---

# 2부. Request for Quotation (English)

## 1. Overview

| Item | Detail |
|---|---|
| Company | [Company / Contact / Phone / Email] |
| Product | Handheld scalp inspection camera (WiFi, smartphone app) |
| Request | Macro optics for close-up scalp imaging (Option 1 or Option 2 below) |
| Use | Observation and recording of pores, hair shafts and scalp surface (not for medical diagnosis) |
| Date | [YYYY-MM-DD] / Reply by [YYYY-MM-DD] |

## 2. Target cameras

| | Camera A | Camera B |
|---|---|---|
| Sensor | Sony IMX708 (12 MP, 4608×2592, 1.4 µm) | Sony IMX219 (8 MP, 3280×2464, 1.12 µm) |
| Active area | 6.45 × 3.63 mm | 3.67 × 2.76 mm |
| Stock lens | Raspberry Pi Camera Module 3 **Wide** (EFL 2.75 mm, F2.2, 120° diagonal) | IMX219 AF module (EFL ≈ 3.0 mm, F2.0 — please confirm) |
| Focus | VCM autofocus (min. focus ≈ 50 mm) | VCM autofocus (min. focus ≈ 100 mm — please confirm) |
| Interface | MIPI CSI-2 | MIPI CSI-2 |

## 3. Target optical specification

| Item | Target | Note |
|---|---|---|
| **Working distance** | **20 mm** | From camera front to scalp surface; all optics must fit inside |
| **Field of view** | **6 mm horizontal** (±5%) | Vertical follows sensor aspect |
| On-screen magnification | ≈ 11× | On a phone screen ≈ 66 mm wide |
| Resolution (object side) | Center ≥ 50 lp/mm (≈ 10 µm), 0.7 field ≥ 35 lp/mm | Hair diameter 50–100 µm |
| Depth of field | ≥ 0.5 mm at the above resolution | |
| Distortion | ≤ 2 % | Used for hair measurement |
| Relative illumination | ≥ 70 % | |
| Spectrum | 420–680 nm (white LED), AR coated | |
| Focus adjustment | VCM stroke covers ≥ ±0.5 mm of working distance | |
| Illumination | Space for an LED ring around the lens | Cross-polarized illumination |

## 4. Options

**Option 1 — Add-on close-up lens in front of the existing camera.**
Our calculation shows that an add-on lens alone cannot fill the full sensor with a 6 mm field within 20 mm total length (see Appendix A).
A realistic target is **12–15 mm optical field + 6 mm center digital crop**.
Please propose lens EFL, diameter, element count, material, spacing (camera↔lens, lens↔object) and achievable MTF in the 6 mm center.

**Option 2 — Camera module with a dedicated macro lens (preferred).**
A complete module with a macro lens designed for 20 mm working distance, mounted on a VCM.

| | Camera A (IMX708) | Camera B (IMX219) |
|---|---|---|
| Optical magnification | ≈ 1.08× | ≈ 0.61× |
| Reference (thin-lens) | EFL ≈ 10.4 mm, sensor–lens ≈ 21.5 mm | EFL ≈ 7.6 mm, sensor–lens ≈ 12.2 mm |
| Sensor-to-object length | ≈ 42 mm | ≈ 32 mm |

- VCM driver IC: **DW9714 or compatible**, on the same I2C bus as the sensor preferred.
- If you already produce **skin / scalp inspection macro modules**, please propose them.

## 5. Mechanical / environment

- Max. optics outer diameter: [e.g. Ø 14 mm]
- Contact window (glass or sapphire, AR coated) included in the design
- Must withstand wiping with alcohol
- Operating temperature: 0–40 °C

## 6. Please provide

1. Optical design summary (layout, EFL, F-number, WD, FOV)
2. MTF (object-side lp/mm), depth of field, distortion, relative illumination
3. 2D drawing and 3D model (STEP)
4. Tolerance analysis / expected yield
5. Unit price: sample / 100 / 500 / 1,000 / 5,000 pcs
6. NRE and tooling cost, MOQ
7. Lead time for samples and mass production
8. Track record of similar products (skin, scalp, intraoral cameras)

---

# 3부. 核心规格 (中文, 1688/阿里巴巴供应商用)

| 项目 | 要求 |
|---|---|
| 用途 | 头皮检测仪 微距摄像头 (头发、毛孔观察) |
| 传感器 | A: IMX708 (12MP) / B: IMX219 (8MP) |
| 工作距离 | **20 mm** (镜头前端到头皮) |
| 视场 | **水平 6 mm** |
| 分辨率 | 物方 ≥ 50 lp/mm (中心) |
| 景深 | ≥ 0.5 mm |
| 畸变 | ≤ 2% |
| 对焦 | VCM 自动对焦 (DW9714 或兼容驱动 IC) |
| 方案 | 方案1: 外加近摄镜片 / **方案2: 定制微距镜头模组 (优先)** |
| 其他 | 镜头周围需留 LED 环形灯位置, 前端保护玻璃 (AR 镀膜) |
| 报价 | 样品 / 100 / 500 / 1000 / 5000 pcs 单价, 开发费, MOQ, 交期 |

---

# 부록 A. 사양 산출 근거

## A-1. "11배, 6 mm"의 관계

두피 스캐너의 배율은 화면에서 본 배율입니다.
스마트폰 세로 화면 폭을 약 66 mm로 보면 **66 ÷ 6 = 11배**입니다.
센서 위에 맺히는 광학 배율은 카메라마다 다릅니다.

| | 센서 가로 | 가로 6 mm를 센서 전체에 담는 광학 배율 |
|---|---|---|
| IMX708 | 6.45 mm | 1.08배 |
| IMX219 | 3.67 mm | 0.61배 |

## A-2. 옵션 1 (보조 렌즈) 계산 결과

조건: 카메라 앞면 ~ 두피 20 mm, 카메라 초점은 AF 범위 중앙
(IMX708 Wide 10 cm, IMX219 AF 20 cm)에 두고 계산했습니다.
"필요 구경"은 중앙 6 mm 영역을 통과시키는 데 필요한 보조 렌즈 지름입니다.

**IMX708 Wide (EFL 2.75 mm)**

| 보조 렌즈 f | 카메라↔렌즈 | 렌즈↔두피 | 광학 시야 | 중앙 6 mm의 가로 픽셀 | 필요 구경 (F값) |
|---|---|---|---|---|---|
| 6 mm | 14.4 mm | 5.6 mm | 14.9 mm | 1,850 | Ø14.8 (F/0.41) — 제작 불가 |
| 10 mm | 11.0 mm | 9.0 mm | 23.0 mm | 1,200 | Ø8.0 (F/1.25) |
| 12 mm | 9.4 mm | 10.6 mm | 26.7 mm | 1,040 | Ø6.2 (F/1.9) |

**IMX219 AF (EFL 약 3.04 mm 가정)**

| 보조 렌즈 f | 카메라↔렌즈 | 렌즈↔두피 | 광학 시야 | 중앙 6 mm의 가로 픽셀 | 필요 구경 (F값) |
|---|---|---|---|---|---|
| 6 mm | 14.2 mm | 5.8 mm | 7.4 mm | 2,640 | Ø15.3 (F/0.39) — 제작 불가 |
| 10 mm | 10.5 mm | 9.5 mm | 11.9 mm | 1,650 | Ø7.9 (F/1.27) |
| 12 mm | 8.7 mm | 11.3 mm | 14.1 mm | 1,400 | Ø6.0 (F/2.0) |

결론:

- 20 mm 안에서 시야를 6 mm로 좁히려면 보조 렌즈가 두피 쪽으로 가야 하고, 그러면
  카메라와 멀어져 렌즈 지름이 초점거리보다 커집니다(F/1 미만). 현실적으로 만들 수 없습니다.
- **IMX708 Wide는 120° 광각이라 보조 렌즈 방식에 특히 불리합니다.**
  광학 시야가 23 mm 이상으로 넓어지고, 중앙 6 mm에 남는 픽셀이 약 1,000~1,200개뿐입니다.
- IMX219 AF가 상대적으로 낫습니다(중앙 6 mm에 약 1,400~1,650 픽셀).
- 보조 렌즈 방식에서 AF가 보정하는 두피 거리 범위는 IMX219 AF가 약 ±0.25~0.35 mm,
  IMX708 Wide가 약 ±0.5~0.7 mm로 좁습니다. 거리 고정 경통이 필수입니다.

## A-3. 옵션 2 (매크로 전용 렌즈) 계산 결과

단렌즈 근사로 계산했습니다(작동거리 u = 20 mm).

| | IMX708 | IMX219 |
|---|---|---|
| 광학 배율 m | 1.075 | 0.612 |
| 렌즈 EFL = m·u/(1+m) | 10.4 mm | 7.6 mm |
| 센서~렌즈 = m·u | 21.5 mm | 12.2 mm |
| 센서 픽셀의 피사체 크기 | 1.3 µm | 1.8 µm |
| VCM 0.25 mm 이동 시 초점 이동 | 약 0.22 mm | 약 0.67 mm |

- 센서 전체 해상도를 6 mm 시야에 쓸 수 있습니다.
- IMX219는 배율이 낮아서 **VCM으로 보정되는 거리 범위가 3배 넓습니다.** 두피 스캐너에는 유리합니다.

## A-4. 해상력과 피사계 심도는 서로 맞바꿉니다

피사체 쪽 개구수(NA)에 따라 대략 이렇게 정해집니다 (λ = 0.55 µm).

| 목표 해상력 | 필요한 NA | 피사계 심도 |
|---|---|---|
| 5 µm | 약 0.067 | 약 0.12 mm |
| **10 µm** | **약 0.034** | **약 0.5 mm** |
| 20 µm | 약 0.017 | 약 1.9 mm |

- 모발(50~100 µm)과 모공 관찰에는 **10 µm 해상력 + 0.5 mm 심도**가 균형점이라서 3장의 목표값으로 잡았습니다.
- 이 경우 광학 해상력은 가로 약 600 lp(약 1,200 유효 픽셀) 수준입니다.
  8~12MP 센서는 이보다 촘촘하게 샘플링하므로 해상도 여유가 있습니다.
