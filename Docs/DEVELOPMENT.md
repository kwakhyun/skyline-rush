# 실행 및 검증

Windows PC, Unreal Engine 5.8.1, DX12/SM6, High, 1920×1080, 목표 60 FPS가 현재 기준입니다.

```powershell
.\Tools\play_runner.ps1 -Build -Windowed
.\Tools\test_runner.ps1 -Build
.\Tools\qa_runner_visual.ps1 -Mode Review -Tag LocalReview -Wait
.\Tools\qa_runner_visual.ps1 -Mode Profile -Tag LocalProfile -ShowGame -Wait
```

QA 결과는 Saved/QA 아래에 저장됩니다. 기존 태그를 덮어쓰지 않습니다. Profile은 실제 렌더러에서 수행하며 NullRHI 테스트 결과를 GPU 성능으로 해석하지 않습니다. QA 전용 저장 슬롯을 사용합니다. 온라인 공개 랭킹에 테스트 기록을 보내지 않습니다.

에디터에서 Tools/Unreal/audit_assets.py를 실행하면 현재 에셋 의존성·누락·미사용 후보를 Saved/QA/asset-dependencies.json에 기록합니다. 미사용 후보 자동 삭제 기능은 없습니다.

캐릭터는 SourceArt/Characters/StudentHero/IllustrationV2/StudentIllustration_Source.blend를 편집하거나 Blender에서 Tools/Character/build_student_illustration.py를 실행합니다. 해당 재생성 스크립트는 보존된 v3 Blender 입력을 사용하며 현재 출력 FBX와 Blender 파일을 갱신하므로 작업 중인 원본은 먼저 별도로 저장하세요. Unreal 재임포트는 현재 캐릭터 에셋에 적용하며 기존 스켈레톤을 유지합니다.

Tools/Unreal/build_biome_assets.py는 현재 폴더에 없는 기본 테마 에셋을 생성합니다. 이미 있는 머티리얼을 임의로 교체하지 않습니다. 나무는 보존된 CityIllustration_Source.blend에서 필요한 StreetTree 두 변형만 내보냅니다.

랭킹 서비스 로컬 빌드:
```powershell
cd Server/Ranking
npm ci
npm run build
```

배포 설정·로컬 DB와 별도 Git 기록은 보존했습니다. 이번 작업에서 서비스를 재배포하지 않았습니다. node_modules, dist, Unreal Intermediate/Binaries/Saved는 재생성 가능하며 Git에서 제외합니다. 에셋과 원본 바이너리는 Git LFS를 사용합니다.

패키징은 SkylineRush 게임 타깃과 L_SkylineRush 맵을 사용합니다. 에디터 Standalone 검증과 Shipping 패키징 검증은 별개입니다.

