# syntax=docker/dockerfile:1

FROM python:3.8.16-bullseye
WORKDIR /srv/jekyll

RUN apt-get update && apt-get install -y ruby-full nodejs npm
RUN gem install bundler

COPY requirements.txt .
COPY Gemfile .

RUN pip install -r requirements.txt
RUN bundle install
RUN npm install livereload

CMD ["bash", "bin/build_me.sh"]
