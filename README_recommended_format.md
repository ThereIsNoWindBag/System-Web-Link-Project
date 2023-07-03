# KDT 토이 프로젝트

   여기에 그림을.....


# 디렉토리 구조

* /backend
    backend 소스 코드(golang 기반)

* /bin
    toy_system + toy_web 프로그램 실행 파일 위치 (기존에는 filebrowser 사용)

* /systems
    우리의 system 관련 프로그램 소스 코드
    엔진/카메라/센서 등등

* /drivers
    앞으로 토이 프로젝트에서 만들 디바이스 드라이버 코드

* /kernel
    빌트인 드라이버 + 커널 디버깅을 위한 스크립트 또는 커널 관련 수정 사항

* /buildroot
    이번 기수에서 사용하는 빌드 시스템

* /frontend
    Frontend 소스 코드(React 기반)


# 타겟 버전 빌드 방법

* 전체 빌드

```sh
make toy
```

* system
```sh
make system
```

* Frontend
```sh
make fe
```

* Backend
```sh
make be
```

# PC 디버깅 버전 빌드 방법

* 전체 빌드

```sh
make toy
```

* system
```sh
make system
```

* Frontend
```sh
make fe
```

* Backend
```sh
make be
```

# system 개발 환경 구축 