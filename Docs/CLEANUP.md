# 프로젝트 정리 — 2026-09-07

현재 프로젝트: C:/Users/82105/Documents/SkylineRushUE/SkylineRush.uproject

## 변경 내용

- HumanOverride 프로젝트·모듈·타깃을 SkylineRush / SkylineRushEditor로 변경했습니다.
- C++를 Gameplay, World, UI, Online, QA, Tests로 정리했습니다. 구형 전투·보스·스토리·무기·성장 시스템 소스를 제거했습니다.
- Unreal AssetRegistry로 실제 C++ 로드 경로와 시작 맵의 하드·소프트 의존성을 추적했습니다. 프로젝트 패키지는 938개에서 103개로 정리됐습니다. 현재 누락 0개, 미사용 0개입니다.
- 에셋은 Unreal AssetTools로 이동하고 참조를 저장했습니다. 맵은 새 경로에 저장했습니다. 현재 에셋에 연결된 원본 51개는 새 SourceArt 경로로 갱신했습니다.
- 구형 기획서·도시 스토리 시안·전투 이미지·중복 디자인 버전·과거 검증 결과·중간 백업·오래된 실행 파일 및 Unreal 캐시를 제거했습니다. 총 6,382개 파일 제거 작업에는 임시 생성 파일도 포함됩니다.
- 편집 가능한 현재 캐릭터·나무 Blender 원본, 사용 중인 FBX·텍스처·음원, VRoid 메타데이터와 출처를 보존했습니다.
- 화면에 표시되지 않던 배경·수중 효과·오라 풀을 제거해 2,332개 인스턴스와 10개 ISM 컴포넌트를 줄였습니다. 결정론적 Types 소스는 include 경로와 모듈 API 접두사 외에는 동일함을 백업과 대조했습니다.
- 랭킹의 미사용 UI 컴포넌트와 의존성을 정리하고 337개 설치 패키지를 제거했습니다. 서비스 코드, DB·마이그레이션·배포 설정과 별도 Git 기록을 보존했습니다.
- 기록 클래스 이름과 CoreRedirect를 유지했습니다. 기존 기록 파일 5개가 바이트 변경 없이 새 모듈에서 로드됐습니다.

폴더 용량은 Git 내부를 제외한 동일 기준으로 약 17.84 GiB → 3.41 GiB입니다. 새 위치에서 검증하며 재생성한 현재 빌드 캐시와 QA 출력은 포함하고, 외부 백업은 제외합니다. 자동 생성 폴더는 Git에 넣지 않습니다.

## 검증

- 새 경로의 SkylineRushEditor Win64 Development 빌드 성공.
- Unreal 러너 자동화 12/12, 온라인 리플레이 대조 3/3, 실제 UI·입력 흐름 21/21 통과.
- 랭킹 서비스 의존성 재설치 및 빌드 성공. 공개 랭킹에 테스트 기록을 전송하거나 서비스를 재배포하지 않았습니다.
- 1920×1080, DX12 High, 100% 내부 해상도, 시드 409, 60Hz 고정 시뮬레이션, 60초 구간으로 실제 렌더링을 측정했습니다. 측정 프레임 전체가 foreground였습니다.

| 항목 | 정리 전 | 정리 후 |
|---|---:|---:|
| 평균 FPS | 102.96 | 103.90 |
| 프레임 P95 (ms) | 14.05 | 13.94 |
| Game Thread (ms) | 2.96 | 2.42 |
| Render Thread (ms) | 9.70 | 9.61 |
| GPU (ms) | 8.99 | 8.98 |
| Draw calls 평균 | 240.7 | 216.1 |
| 인스턴스 | 13164 | 10832 |
| ISM 컴포넌트 | 56 | 46 |

평균 및 P95가 60 FPS 예산을 충족합니다. 한 PC의 단일 반복 측정이므로 작은 FPS 차이를 확정적인 향상으로 해석하지 않습니다. 첫 실행 CleanupFinal은 포커스가 바뀌어 84.12 FPS / P95 19.02ms였고, 비교에는 전체 3,260프레임이 foreground였던 CleanupProfile을 사용했습니다. 첫 측정도 Saved/QA에 남겼습니다.

현재 화면: [플레이](Media/gameplay.png), [메인](Media/main.png), [일시정지](Media/pause.png), [결과](Media/result.png), [다섯 테마](Media/five-worlds.png). [기계 판독 검증 요약](QA/validation.json), [에셋 의존성](QA/asset-dependencies.json), [세부 프로파일](QA/profile-summary.json).

Shipping 패키징과 다른 PC에서의 실행은 이번 작업에서 검증하지 않았습니다. 보존된 Blender 제작 도구는 구문 검사를 했으며 모델을 재생성하지는 않았습니다.

## 새 구조와 유지 규칙

README 및 ARCHITECTURE.md를 기준으로 작업합니다. 이전 버전 산출물과 대형 롤백 ZIP을 프로젝트 안에 다시 쌓지 않습니다. QA 출력은 Saved/QA, 채택한 최신 화면·요약만 Docs에 저장합니다. 에셋 삭제 전 Tools/Unreal/audit_assets.py로 동적 로드와 의존성을 검사합니다.

기존 HOO UCLASS와 일부 에셋의 v1/v2 이름은 호환성과 출처 식별 때문에 남겼습니다. 온라인 규칙 버전과 저장 슬롯을 폴더 이름 정리와 함께 변경하지 않습니다.

기존 HumanOverrideOverloadUE 폴더는 Windows에서 사용 중인 것으로 표시되어 직접 이름 변경이 실패했습니다. 프로젝트 내용 전체를 새 SkylineRushUE 폴더로 이동했으며 이전 폴더에는 파일이 없습니다. 앱에서 새 폴더를 열고 이전 폴더 사용을 끝내면 빈 폴더를 삭제할 수 있습니다.

## 복원

외부 백업: C:/Users/82105/Documents/SkylineRushCleanupBackup_20260907/ProjectBeforeCleanup.zip

정리 전 소스·에셋·문서·개인 기록 2,226개가 포함되어 있고 ZIP CRC 전체 검증을 통과했습니다. Unreal 생성 캐시와 서버 node_modules는 백업하지 않았습니다. 루트 Git 기록은 보존된 현재 프로젝트의 .git에 있고, 로컬 서버 DB 상태는 현재 Server/Ranking/.wrangler에 보존됩니다.

게임과 에디터를 종료한 후 ZIP을 별도의 새 복원 폴더에 풀어 사용하세요. 현재 프로젝트를 덮어쓰거나 .git, 로컬 DB 및 이후 플레이 기록을 삭제하지 마세요. 필요하면 백업에서 특정 파일만 골라 복원할 수 있습니다. 원본 파일 목록·에셋 경로 매핑·삭제 목록도 같은 외부 백업 폴더에 있습니다.

실행·빌드·검증 명령은 [DEVELOPMENT.md](DEVELOPMENT.md)에 있습니다.
