# SKYLINE RUSH

은발 소녀가 세 레인을 달리는 Unreal Engine 5.8 PC 러닝 게임입니다. 자연숲·도심·건물 내부·정글·차원 공간을 통과하며 가속, 코너, 점프, 슬라이드, 부스터와 피버를 사용합니다. 목숨은 3개이며 로컬 기록과 온라인 랭킹을 지원합니다.

## 실행

[Windows 프리뷰 다운로드](https://github.com/kwakhyun/skyline-rush/releases/tag/v0.7.0-preview.2) · [온라인 랭킹](https://skyline-rush-ranking.khyun97.chatgpt.site)

Windows 패키지는 ZIP 전체를 풀고 `SkylineRush.exe`를 실행합니다. Unreal Editor 설치는 필요하지 않습니다.

**공개 저장소 안내:** 전체 게임·서버·도구 소스코드를 제공합니다. 재배포 제한이 있는 원본 모델과 Unreal 에셋은 포함하지 않으므로 저장소만 복제하면 동일한 게임을 실행할 수 없습니다. [공개 범위와 패키징 안내](Docs/PUBLISHING.md)를 확인하세요.

소스에서 빌드하려면 Unreal Engine 5.8과 Visual Studio C++ 개발 도구가 필요합니다. 프로젝트 파일은 **SkylineRush.uproject**입니다.

```powershell
.\Tools\play_runner.ps1 -Build -Windowed
```

Space: 시작/점프 · A/D 또는 ←/→: 레인 변경 · S 또는 ↓: 슬라이드 · Esc: 일시정지 · F1: 설정 · F2: 기록/랭킹 · R: 새 코스.

Breezy Adventure 두 곡이 차례로 반복됩니다. 일시정지하면 음악도 멈추고, 재도전 시에는 듣던 곡이 이어집니다. F1의 소리 크기 설정으로 효과음과 음악 음량을 함께 조절합니다.

수집·피격·부스터·피버에는 각각 전용 효과음을 사용합니다. 긴 생성 음원을 짧게 편집하고 음량·끝부분을 보정했으며, 수집음이 과하게 겹치지 않도록 제한합니다.

다섯 테마에 곡면 거목·강철 공사장·대리석 보안 홀·비취 유적·차원 관문 특별 구간을 구현했습니다. 청록색 안전 경로와 금색 도전 경로를 선택하고 점프·슬라이드·틈 통과 보상, 아슬아슬 점수를 획득합니다. 결과 화면에서 네 가지 점수 항목을 확인할 수 있습니다. [특별 구간 규칙](Docs/SPECIAL_SECTIONS.md) · [실제 화면과 검증](Docs/QA/special-sections.md)

## 구조

| 폴더 | 내용 |
|---|---|
| Source/SkylineRush | 게임플레이, 월드, UI, 온라인, QA, 테스트 C++ |
| Content/SkylineRush | 현재 맵과 사용 중인 Unreal 에셋 |
| SourceArt | 캐릭터·환경 편집 원본, 텍스처·음원·출처 |
| Server/Ranking | 온라인 랭킹 서비스와 리플레이 검증 |
| Tools | 실행, 테스트, 프로파일링, 에셋 제작·검사 도구 |
| Docs | 현재 게임 문서와 검증 근거 |
| Saved | 개인 저장 기록과 자동 생성 QA 출력; Git 제외 |

[게임 규칙](Docs/GAMEPLAY.md) · [코드 구조](Docs/ARCHITECTURE.md) · [개발·검증](Docs/DEVELOPMENT.md) · [에셋 출처](Docs/ASSET_PROVENANCE.md) · [정리 내역·복원](Docs/CLEANUP.md)

![다섯 테마 실제 게임 화면](Docs/Media/five-worlds.png)

## 전용 타이틀 화면

은발 주인공과 하늘 도시의 전용 일러스트, 최고 기록과 시작·설정·기록 메뉴를 적용했습니다. [구현과 검증](Docs/TITLE_SCREEN.md)

![타이틀 화면](Docs/Media/Title/16x9.png)

승인된 두 번째 아트 시안을 현재 소스에 적용했습니다. 타이틀 로고는 이미지로 표시하며 설정·기록 화면에는 글자 없는 일러스트를 사용합니다. 이 소스 변경은 위 v0.7.0-preview.2 공개 실행 파일 이후의 업데이트입니다.
