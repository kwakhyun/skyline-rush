# 타이틀 화면

기존 은발 주인공을 참고한 SKYLINE RUSH 전용 일러스트를 적용했습니다. 오른쪽에는 노을 속 하늘 도시를 달리는 주인공, 왼쪽에는 제목과 메뉴를 배치합니다.

- 달리기 시작, 기록과 랭킹, 게임 설정, 새 코스, 종료의 기존 동작을 연결합니다.
- 최고 점수와 최고 거리를 표시합니다.
- Ready 상태에서만 전용 그림을 그립니다. 타이틀에서 설정·기록·닉네임 창을 열면 그림 위에 표시하며 주행 정보는 숨깁니다.
- 시작하면 실시간 3D 화면으로 전환합니다. 일시정지·결과 화면은 기존 주행 배경과 점수 분해를 유지하고, 처음 화면으로 돌아오면 일러스트가 다시 나타납니다.
- 이미지의 세로 비율을 유지합니다. 넓은 화면에서는 오른쪽 정렬과 왼쪽 가장자리 확장, 좁은 화면에서는 좌우 중앙 크롭으로 대응합니다.

## 이미지와 구현

원본: `SourceArt/UI/Title/T_SkylineRush_Title_v1.png` (1672 × 941). Built-in Imagegen으로 참고 이미지 기반 생성했으며 원본 픽셀을 수정하지 않았습니다. 제목과 버튼은 런타임 UI입니다. 전체 프롬프트·참고 자료·SHA256은 `SourceArt/UI/Title/PROVENANCE.md`에 있습니다.

런타임 텍스처: `/Game/SkylineRush/UI/Title/T_SkylineRush_Title_v1`. UI 압축, sRGB, 밉맵 없음, 상시 상주, 가장자리 Clamp를 사용합니다. 가져오기 스크립트는 `Tools/Unreal/import_runner_title.py`입니다.

`HOORunnerGameMode.cpp`의 HUD가 전체 화면 그림을 그리며 `HOORunnerMenuWidget`이 메뉴를 표시합니다. 기존 메뉴 위젯을 재사용하므로 별도 맵이나 게임 모드로 우회하지 않습니다.

## 검증

Editor 컴파일, 러너 자동화 테스트 13개와 오프라인 서버 리플레이 대조가 통과했습니다. 실제 GPU 실행에서 1280×720, 1280×960, 1720×720의 타이틀·설정·기록·시작·일시정지·복귀 화면을 캡처했습니다. 각 해상도에서 텍스처 상주, 메뉴 구성, 버튼 델리게이트 동작과 상태 전환 등 8개 검증을 통과했습니다. 창 크기별 결과는 `Docs/QA/title-screen.json`, 대표 화면은 `Docs/Media/Title`에 있습니다.

```powershell
.\Tools\qa_runner_visual.ps1 -Mode Title -Tag Title_NewCheck -Width 1280 -Height 720 -Wait
```

검증 결과는 `Saved/QA/<Tag>/title-checks.json`에 기록됩니다. QA는 별도 저장 슬롯을 사용합니다.
