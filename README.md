# VENTUNO Q RC카

Arduino VENTUNO Q용 키보드 주행 테스트. Ubuntu에서 조종 페이지를 열고, STM32가 후륜 모터 두 개와 앞바퀴 조향 서보를 제어한다. 화면에서 양쪽 엔코더 누적 카운트를 확인할 수 있다.

## 실행

1. [릴리스의 App Lab ZIP](https://github.com/Choihyunseok1/ventuno-q-rc-car/releases/latest)을 내려받아 Arduino App Lab의 **Import App**으로 가져온다. VENTUNO Q를 선택하고 **Run**을 누른다.
2. PC와 보드를 같은 신뢰할 수 있는 네트워크에 연결한다. PC 브라우저에서 `http://<보드-IP>:8765`를 열고, App Lab의 **Python** 콘솔에 표시되는 조종 코드를 입력한다. 보드에서 직접 열 때는 `http://localhost:8765`를 사용한다.
3. 차를 들어 바퀴를 띄운 상태에서 방향과 조향 간섭을 먼저 확인한다. 펌웨어의 `kLeftDriveSign` / `kRightDriveSign`은 모터 방향, `kLeftEncoderSign` / `kRightEncoderSign`은 엔코더 부호, `kSteerCenterUs` / `kSteerSign`은 조향 중립과 방향을 바꾼다.

| 입력 | 동작 |
| --- | --- |
| `W` / `S` 또는 `↑` / `↓` | 누르고 있는 동안 전진 / 후진 |
| `A` / `D` 또는 `←` / `→` | 누르고 있는 동안 좌 / 우 조향 |
| `Space` | 구동 정지, 조향 중립 |
| `Esc` | 정지 및 조종 해제 |

테스트 듀티는 약 30%이며 출발 때 서서히 올린다. 탭이 포커스를 잃으면 조종을 해제한다. STM32는 유효 명령이 300ms 동안 없으면 구동을 멈추고 조향을 중립으로 돌린다. 조종 코드는 앱을 시작할 때마다 새로 발급된다.

## 차량과 회로

| 파일 | 용도 |
| --- | --- |
| [실측 반영 메인바디 STEP](hardware/chassis.step) · [STL](hardware/chassis.stl) | 차체 수정 및 출력 |
| [나머지 출력 부품 3MF](hardware/print-parts-except-chassis.3mf) | 커버, 모터 클램프, 너클, 조향 링크 등 |
| [전체 조립 참고 STEP](hardware/vehicle-assembly-reference.step) | 전후륜, 차체, 배터리 등 배치 참고. 11월 25일 실측 반영 메인바디는 위 개별 파일 사용 |
| [듀얼 배터리 배선도 SVG](hardware/dual-battery-wiring.svg) · [PNG](hardware/dual-battery-wiring.png) | 커넥터와 전원, STM32 핀 배치 |

| VENTUNO Q | 연결 |
| --- | --- |
| D3 / D4 | MDD3A M1A PWM / M1B 방향, 왼쪽 후륜 |
| D6 / D7 | MDD3A M2A PWM / M2B 방향, 오른쪽 후륜 |
| D9 | MG90S 신호. 전원은 보드 5V와 GND |
| D2 / D8 | 왼쪽 엔코더 A / B |
| D11 / D12 | 오른쪽 엔코더 A / B |
| 3.3V / GND | 양쪽 엔코더 전원 |

3S 팩 A는 VENTUNO Q DC 잭, 팩 B는 MDD3A `VB+` / `VB−`에 연결한다. J8/2 GND와 드라이버 신호 GND를 연결해 기준 전위를 맞춘다. 팩 양극은 서로 분리하고 J8/1 VIN과 MDD3A 5VO는 비워 둔다.

[Arduino App Lab 사용법](https://docs.arduino.cc/software/app-lab/apps/manage-apps) · [VENTUNO Q MCU 핀 제어](https://docs.arduino.cc/tutorials/ventuno-q/mcu-examples/) · [Cytron MDD3A](https://www.cytron.io/p-3amp-4v-16v-dc-motor-driver-2-channels)
