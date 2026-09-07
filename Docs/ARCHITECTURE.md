# 구현 구조

런타임 모듈과 빌드 타깃은 SkylineRush / SkylineRushEditor입니다. 시작 맵은 /Game/SkylineRush/Maps/L_SkylineRush이며 게임 모드는 HOORunnerGameMode입니다.

| 코드 영역 | 역할 |
|---|---|
| Public·Private/Gameplay | 결정론적 Types, Pawn, GameMode·Controller·기록 |
| Public·Private/World | Scenery 효과·배경, Worlds 다섯 테마 및 고정 크기 인스턴스 풀 |
| Public·Private/UI | 공통 스타일·위젯, HUD, 메뉴·결과 |
| Private/Online | Pawn 온라인 등록·전송·랭킹 연동 |
| Public·Private/QA | 실제 Unreal 실행 기반 화면·성능·메뉴 검증 |
| Private/Tests | 코스 도달 가능성, 시뮬레이션, 저장·기록, 리플레이 테스트 |

공개 헤더는 영역 경로로 포함합니다. 물리 규칙은 Gameplay/HOORunnerTypes에 집중되어 있고 서버의 Server/Ranking/lib/simulation.mjs와 오프라인 대조합니다. 슬롯/메시 인스턴스 개수는 유한합니다.

기존 UCLASS 이름의 HOO 접두사는 저장 데이터 호환성을 위해 유지했습니다. Config/DefaultEngine.ini의 CoreRedirects가 /Script/HumanOverrideOverload를 /Script/SkylineRush로 연결합니다. 옛 모듈 소스나 DLL은 필요하지 않습니다.

에셋은 Characters/{Animations,Materials,Meshes,Rig,Textures}, Environment/{Materials,Meshes,Textures}, Audio, UI, Maps로 나눴습니다. ConstructorHelpers, 명시적 LoadObject, 동적으로 구성하는 애니메이션 경로와 맵을 시작점으로 AssetRegistry의 하드·소프트 의존성을 추적했습니다.

원본에 v1/v2가 붙어 있어도 현재 머티리얼·스켈레톤이 참조하는 파일은 유지합니다. 파일명만 보고 지우지 말고 Tools/Unreal/audit_assets.py와 재빌드·실행으로 확인합니다.

