# SLOWER Local Actuator Monitor (PlatformIO)

## 핵심 방향
이 프로젝트는 **서버 조회형이 아니라 내부 상태 표시형**입니다.

즉,
- 기존 KSX/RS485 처리부가 이미 알고 있는 구동기 상태를
- `deviceId` 기준으로 레지스트리에 반영하고
- 지정한 구동기만 1.85" LCD에 표시합니다.

## 현재 표시 대상
- 222 SW1
- 223 SW2
- 228 OP1
- 229 OP2

## 현재 포함된 것
- Wi-Fi 관리
- 원형 LCD 표시
- 로컬 구동기 상태 레지스트리
- 상태 변화 감지
- 음성 이벤트 훅 (`query`, `update`, `fail`)
- 기존 서버 보고 함수 보존 (`web_status_bridge.*`)

## 아주 중요한 구조
실제 상태 입력은 `local_state_bridge.*`로 들어갑니다.

기존 코드의 KSX 처리부에서 아래 둘 중 하나를 호출하면 됩니다.

```cpp
LocalStateBridge::onActuatorStateObserved(deviceId, command, standardStatus, remainSec);
LocalStateBridge::onActuatorWritten(deviceId, command, standardStatus, remainSec);
```

### 연결 권장 위치
- `ksxPoll...` 류에서 실제 상태 읽은 직후 → `onActuatorStateObserved(...)`
- `ksxWriteActuatorState(...)` 성공 직후 → `onActuatorWritten(...)`

## 왜 이 구조가 맞는가
기존 업로드 코드에서:
- 개별 구동기 상태 보고는 `id=deviceId` 기반
- 보드 상태 보고는 `farmIndex` 기반

따라서 LCD에 개별 구동기를 띄우는 현재 목적에는 `farmIndex` 조회보다 `deviceId` 중심 로컬 상태 구조가 더 자연스럽습니다.

## 음성
기본값은 **영어 텍스트 이벤트 + 비프 fallback**입니다.

시리얼 로그 예:
- `[VOICE] query`
- `[VOICE] update`
- `[VOICE] fail`

실제 spoken English WAV 재생은 보드의 스피커 경로를 정확히 맞춘 뒤 `audio_manager.cpp`만 교체하면 됩니다.

## 시작 순서
1. 업로드 후 LCD 동작 확인
2. `g_demoEnabled = false`로 변경
3. 기존 KSX 처리 코드에서 `LocalStateBridge::onActuatorStateObserved(...)` 연결
4. 필요 시 `web_status_bridge.*`로 서버 상태 보고 유지

## 첫 통합 예시
```cpp
// 예: 폴링으로 실제 상태 읽은 직후
LocalStateBridge::onActuatorStateObserved(228, 301, 301, 0);

// 예: 개폐기 timed open 명령 성공 직후
LocalStateBridge::onActuatorWritten(228, 303, 303, 15);
```
