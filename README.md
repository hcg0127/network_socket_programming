# network_socket_programming
## 2024 3-1 컴퓨터네트워크 과제 - 소켓 프로그래밍 설명
- './myserver <port number>'을 입력해 서버를 연다.
- 서버와 클라이언트가 같은 컴퓨터에서 돌아가기 때문에 클라이언트는 Firefox에서 localhost나 127.0.0.1 뒤에 자신이 입력한 port number를 붙여 서버에 요청을 하게 된다.
- 뒤에 '/'만 올 경우 default.html으로 응답하게 했고, myserver.c 코드와 같은 디렉터리에 있는 jpg, pdf, html 등을 요청하면 서버에서 요청을 parsing하고 그에 맞는 응답을 보낸다.
- 결과적으론 클라이언트가 요청한 파일을 서버가 화면에 띄워줄 수 있다.
- 결과 예시
![image](https://github.com/hcg0127/network_socket_programming/assets/115148838/940a1767-da89-4253-b06f-1d513137d254)
