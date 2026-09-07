# 특별 구간 검증 — 2026-09-07

- Unreal Editor Development 컴파일 성공.
- `HOO.Runner` 자동 검증 13개 성공, 실패 0. 기존 16개 시드의 3.5km 최대 부스터 안전 경로 검증 유지.
- 추가 규칙 검증: 16개 시드 × 20주기 × 5구간의 중앙 안전 차선·3종 도전 배치·36m 경고 거리. 점프/슬라이드/틈의 정확 통과, 피격·피버 보상 제외, 한 장애물 중복 지급 방지, 일시정지, 재시작 초기화.
- Unreal이 내보낸 리플레이 4개를 JavaScript 서버와 대조: 점수·거리·결정·피버·비행 및 추가 보상 항목 일치. 3.5km 시드 409/410/411과 3회 피격 종료 주행 포함.
- 실제 DX12 standalone 렌더러에서 시드 411의 10,796틱 입력 재생 성공: 3,500.07m, 30,820점, 도전 성공 14회, 아슬아슬 6회.
- 결과 항목: 거리 3,500 + 수집 23,120 + 위험 보상 3,440 + 아슬아슬 760 = 30,820. 검증 주행을 끝낸 뒤 QA에서 결과 화면을 열어 표시를 확인했다. 실제 사망 장면의 캡처는 아니다.
- 다섯 특별 구간을 1280×720에서 캡처·검수. 안내판 겹침 수정, 거목 곡면과 유적 아치 개선 후 다시 촬영.
- 에셋 의존성 검사: 누락 0, 활성 에셋은 `/Game/SkylineRush` 내부.
- 로컬 랭킹 API: 인증, 변조 점수 거절, 중복 등록, 정렬, QA/공개 시즌 격리 확인. 이전 Rules 6 리플레이 3개도 계속 검증 가능.
- 위 주행은 고정 시뮬레이션 입력 재생이다. 실제 60fps 성능 보장, 장시간 수동 플레이 평가, 다양한 PC 호환성 검증을 의미하지 않는다.

## 실제 게임 캡처

![거목 통로](../Media/SpecialSections/special-forest.png)
![도심 공사장](../Media/SpecialSections/special-city.png)
![아트리움](../Media/SpecialSections/special-atrium.png)
![비취 유적](../Media/SpecialSections/special-jungle.png)
![차원 관문](../Media/SpecialSections/special-dimension.png)
![점수 분해](../Media/SpecialSections/special-results.png)

원시 로그와 리플레이는 `Saved/QA`에 보관한다. QA는 별도 저장 슬롯을 사용하며 공개 랭킹에 테스트 점수를 전송하지 않는다.
