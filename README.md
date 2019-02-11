# claud
A simple Mail.Ru Cloud client written in C language.

Implementation of [mail.ru cloud](https://cloud.mail.ru/) API.

The functionality is loosely based on the [GoMailRuCloud](https://github.com/sergevs/GoMailRuCloud) project.
This project embeds the code of the [JSMN](https://github.com/zserge/jsmn) project .

# Prerequisites

To build the code, you'll need the libcurl development package.
For Debian-bases distributions, you can install it like this:

    sudo apt install libcurl-dev

To build documentation, doxygen and graphviz are needed. On Debian-based distributions,
You can install these tools using the following command:

    sudo apt install doxygen graphviz

# Installation & Usage

    git clone https://github.com/nikolai-kopanygin/claud
    cd claud
    make
    sudo make install
    export MAILRU_USER=<your mail.ru username>
    export MAILRU_PASSWORD=<your mail.ru password>
    claud --help

# Building documentation

    make doc

# Uninstall

    sudo make uninstall



# API reference
See also [GoMailRuCloud API documentation](https://godoc.org/github.com/sergevs/GoMailRuCloud/Api)
