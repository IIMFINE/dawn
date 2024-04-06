FROM ubuntu:jammy AS build_base

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

ENTRYPOINT [ "/bin/bash" ]

# It can not work
# CMD [ "/bin/bash", "echo '127.0.0.1 docker-desktop' >> /etc/hosts " ]

FROM dawn_base AS build_dependence

ARG USER_NAME=DAWN
ARG SPDLOG_REPO=https://github.com/gabime/spdlog.git
ARG GTEST_REPO=https://github.com/google/googletest.git
ENV DEBIAN_FRONTEND=noninteractive

# Set proxy for git clone
ARG HTTP_PROXY
ARG HTTPS_PROXY
ENV http_proxy=$HTTP_PROXY
ENV https_proxy=$HTTPS_PROXY

USER root

#github repository can be changed. Now it is a proxy hub for China user.
RUN git clone ${SPDLOG_REPO} \
  && mkdir /spdlog/build \
  && cd /spdlog/build && cmake .. && make -j$(nproc) && make install -j$(nproc)\
  && rm -rf /spdlog

#github repository can be changed. Now it is a proxy hub for China user.
RUN git clone ${GTEST_REPO} \
  && mkdir /googletest/build \
  && cd /googletest/build && cmake .. && make -j$(nproc) && make install -j$(nproc)\
  && rm -rf /googletest

USER $USER_NAME
WORKDIR /home/$USER_NAME

ENTRYPOINT [ "/bin/bash" ]
