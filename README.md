# claud
A simple Mail.Ru Cloud client written in C language.

Implementation of [mail.ru cloud](https://cloud.mail.ru/) API.

The functionality is loosely based on the [GoMailRuCloud](https://github.com/sergevs/GoMailRuCloud) project.
This project embeds the code of the [JSMN](https://github.com/zserge/jsmn) project .

# Installation & Usage

    sudo apt install libcurl-dev
    git clone https://github.com/nikolai-kopanygin/claud
    cd claud
    cd src
    mkdir obj
    make
    export MAILRU_USER=<your mail.ru username>
    export MAILRU_PASSWORD=<your mail.ru password>
    ./claud --help


# API reference
See also [GoMailRuCloud API documentation](https://godoc.org/github.com/sergevs/GoMailRuCloud/Api)
