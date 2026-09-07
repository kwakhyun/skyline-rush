# SKYLINE RUSH

은발 소녀가 세 레인을 달리는 Unreal Engine 5.8 PC 러닝 게임입니다. 자연숲·도심·건물 내부·정글·차원 공간을 통과하며 가속, 코너, 점프, 슬라이드, 부스터와 피버를 사용합니다. 목숨은 3개이며 로컬 기록과 온라인 랭킹을 지원합니다.

## 실행

[Windows 프리뷰 다운로드](https://github.com/kwakhyun/skyline-rush/releases/tag/v0.6.0-preview.1) · [온라인 랭킹](https://skyline-rush-ranking.khyun97.chatgpt.site)

Windows 패키지는 ZIP 전체를 풀고 `SkylineRush.exe`를 실행합니다. Unreal Editor 설치는 필요하지 않습니다.

**공개 저장소 안내:** 전체 게임·서버·도구 소스코드를 제공합니다. 재배포 제한이 있는 원본 모델과 Unreal 에셋은 포함하지 않으므로 저장소만 복제하면 동일한 게임을 실행할 수 없습니다. [공개 범위와 패키징 안내](Docs/PUBLISHING.md)를 확인하세요.

Unreal Engine 5.8과 Visual Studio C++ 개발 도구가 필요합니다. 프로젝트 파일은 **SkylineRush.uproject**입니다.

```powershell
.\Tools\play_runner.ps1 -Build -Windowed
```

Space: 시작/점프 · A/D 또는 ←/→: 레인 변경 · S 또는 ↓: 슬라이드 · Esc: 일시정지 · F1: 설정 · F2: 기록/랭킹 · R: 새 코스.

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
