# 공개 소스와 배포

공개 저장소는 현재 SKYLINE RUSH의 C++, 랭킹 서버, 제작·검증 도구, 설정과 문서를 담은 독립적인 소스 스냅샷입니다. 이전 게임의 Git 기록과 별도 랭킹 저장소의 Git 내부 파일은 포함하지 않습니다.

캐릭터 VRM 메타데이터에는 원본 모델 단독 재배포 제한이 있습니다. 공개 소스에는 Content의 Unreal 에셋, 편집용 모델과 원본 텍스처·음원을 포함하지 않습니다. 게임 화면과 출처 문서는 유지합니다. 따라서 공개 저장소만 복제해서는 같은 게임 화면을 실행하거나 패키징할 수 없습니다. 게임 실행에는 별도로 제공되는 Windows 패키지, 개발에는 권한이 있는 원본 에셋이 필요합니다. 자세한 출처는 [ASSET_PROVENANCE.md](ASSET_PROVENANCE.md)를 참고하세요.

코드를 공개한다고 해서 별도의 오픈소스 또는 에셋 재배포 라이선스를 부여하지 않습니다. 기존 저작권 표시와 각 의존성의 라이선스를 유지합니다.

## 소스 내보내기

원본 프로젝트에서 다음 명령을 실행합니다. 대상 폴더는 새 경로여야 합니다.

```powershell
.\Tools\export_public_source.ps1 -Destination "$PWD\Saved\QA\PublicSource-next"
```

내보낸 폴더를 별도 Git 저장소로 관리합니다. Docs의 이미지에는 Git LFS를 사용합니다. Server/Ranking의 중첩 저장소는 일반 소스 파일로 포함하며, .openai/hosting.json은 비밀정보가 없는 기존 서비스 식별자와 논리 DB 바인딩만 포함합니다. 새로운 서비스로 배포할 때에는 자신의 Sites 설정을 사용해야 합니다.

## Windows 패키지

원본 에셋이 있는 프로젝트와 Unreal Engine 5.8이 필요합니다.

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun "-project=$PWD\SkylineRush.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook '-map=/Game/SkylineRush/Maps/L_SkylineRush' -stage -pak -iostore -compressed -archive "-archivedirectory=$PWD\Saved\QA\Release" -prereqs -utf8output -unattended
```

배포 파일에는 cooked 게임 데이터와 실행 파일을 담고 편집 원본과 디버그 심볼은 포함하지 않습니다. Steam 등록·판매는 별도 작업입니다.

## 2026-09-07 공개 결과

- 공개 소스: https://github.com/kwakhyun/skyline-rush
- Windows 프리뷰: https://github.com/kwakhyun/skyline-rush/releases/tag/v0.6.0-preview.1
- 온라인 랭킹: https://skyline-rush-ranking.khyun97.chatgpt.site
- Editor 빌드, 러너 테스트 12개, 리플레이 3개 대조, 로컬 랭킹 API 테스트, Shipping 패키징을 통과했습니다.
- Shipping 실행 파일로 시작·달리기·결과 화면을 확인했습니다. 936m, 6,576점까지 진행하고 게임 종료 버튼을 확인했습니다. 긴 플레이와 다양한 PC의 성능·호환성 검증은 포함하지 않습니다.
- 일부 코너·충돌 장면에서 배경이 카메라를 가리는 모습이 관찰되었습니다. 프리뷰의 시각적 개선 사항으로 남아 있습니다.
- Docs/QA/validation.json의 `shipping_package_tested`와 `deployed`는 이전 정리 작업 당시의 기록입니다. 이번 공개 결과는 Docs/QA/publication.json을 참고하세요.

## Breezy Adventure 음악 업데이트

`v0.6.0-preview.2`는 사용자가 제공한 두 곡의 교대 재생을 추가합니다. 기존 소리 크기 설정을 따르며 일시정지·재개에 연동됩니다. 시작·결과 화면과 재도전에서는 듣던 곡을 유지합니다. 원본 WAV는 로컬 프로젝트에 보존하며 공개 소스에는 임포트 도구와 출처·해시를 포함합니다.

전체 곡의 마지막 2초를 실제 오디오 장치에서 재생해 종료 콜백을 검사했고, 무음 상태에서 1→2→1→2→1→2→1→2 전환을 확인했습니다. 이는 원본 길이를 줄이거나 Shipping 재생을 단축하지 않습니다. 검증 기록은 Docs/QA/music-import.json과 Docs/QA/music-playback.json에 있습니다.

## 전용 효과음 업데이트

`v0.6.0-preview.3`는 사용자가 제공한 수집·피격·부스터·피버 효과음을 추가합니다. 원본을 보존하고 0.35 / 0.95 / 1.4 / 1.8초의 별도 게임용 클립을 만들었습니다. 기본 음량·피크와 끝부분 페이드를 보정했으며, 수집음은 최대 두 개까지 겹치게 제한했습니다. 기존 BGM 교대 재생은 유지합니다. 자동 플레이에서 수집 61회, 피격 2회, 마지막 충돌 1회, 부스터 2회, 피버 1회의 에셋 연결을 확인했습니다. 준비 과정과 검증 기록은 SourceArt/Audio/RunnerSFX 및 Docs/QA/sfx-gameplay.json을 참고하세요.
