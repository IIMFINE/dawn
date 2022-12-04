FROM ubuntu:focal AS build_base

ARG USER_NAME=DAWN
ENV DEBIAN_FRONTEND=noninteractive

COPY ./deps /deps
COPY ./deps/sources.list /etc/apt/
RUN apt install /deps/libssl.deb &&\
    apt install /deps/openssl.deb &&\
    apt install /deps/ca_certificate.deb

RUN apt-get update && apt-get install sudo &&\
    apt-get update && apt-get install -y $(cat deps/packages) \
    && rm -r /deps

RUN useradd -ms /bin/bash ${USER_NAME} &&\
    echo "${USER_NAME}:123456"| chpasswd &&\
    adduser ${USER_NAME} sudo

CMD [ "/bin/bash", "echo '127.0.0.1 docker-desktop' >> /etc/hosts " ]

FROM build_base AS build_dependence

RUN git clone https://github.com/gabime/spdlog.git \
    && mkdir /spdlog/build \
    && cd spdlog/build && cmake .. && make -j$(nproc) && make install -j$(nproc)\
    && rm -rf /spdlog

USER $USER_NAME
WORKDIR /home/$USER_NAME

ENTRYPOINT [ "/bin/bash" ]
