import requests
import json
import sys
import os

cpp_keywords = [
    'alignas',
    'alignof',
    'and',
    'and_eq',
    'asm',
    'atomic_cancel',
    'atomic_commit',
    'atomic_noexcept',
    'auto',
    'bitand',
    'bitor',
    'bool',
    'break',
    'case',
    'catch',
    'char',
    'char8_t',
    'char16_t',
    'char32_t',
    'class',
    'compl',
    'concept',
    'const',
    'consteval',
    'constexpr',
    'constinit',
    'const_cast',
    'continue',
    'co_await',
    'co_return',
    'co_yield',
    'decltype',
    'default',
    'delete',
    'do',
    'double',
    'dynamic_cast',
    'else',
    'enum',
    'explicit',
    'export',
    'extern',
    'false',
    'float',
    'for',
    'friend',
    'goto',
    'if',
    'inline',
    'int',
    'long',
    'mutable',
    'namespace',
    'new',
    'noexcept',
    'not',
    'not_eq',
    'nullptr',
    'operator',
    'or',
    'or_eq',
    'private',
    'protected',
    'public',
    'reflexpr',
    'register',
    'reinterpret_cast',
    'requires',
    'return',
    'short',
    'signed',
    'sizeof',
    'static',
    'static_assert',
    'static_cast',
    'struct',
    'switch',
    'synchronized',
    'template',
    'this',
    'thread_local',
    'throw',
    'true',
    'try',
    'typedef',
    'typeid',
    'typename',
    'union',
    'unsigned',
    'using',
    'virtual',
    'void',
    'volatile',
    'wchar_t',
    'while',
    'xor',
    'xor_eq',
    'final',
    'override',
    'transaction_safe',
    'transaction_safe_dynamic',
    'import',
    'module',
    'if',
    'elif',
    'else',
    'endif',
    'ifdef',
    'ifndef',
    'define',
    'undef',
    'include',
    'line',
    'error',
    'pragma',
    'defined',
    '__has_include',
    '__has_cpp_attribute',
    'export',
    'import',
    'module ',
    '_Pragma ',
    'linux'
]

fa_tag = "https://github.com/components/font-awesome/raw/refs/tags/6.5.2"

def main():

    json_file = {}

    if len(sys.argv) == 2:
        with open(sys.argv[1], 'r') as data:
            json_file = json.load(data)
    else:
        # === Download fonts
        open("fa-solid-900.ttf", "wb").write(requests.get(fa_tag + "/webfonts/fa-solid-900.ttf").content)
        open("fa-regular-400.ttf", "wb").write(requests.get(fa_tag + "/webfonts/fa-regular-400.ttf").content)
        open("fa-brands-400.ttf", "wb").write(requests.get(fa_tag + "/webfonts/fa-brands-400.ttf").content)

        # === Extract icons data directly from the source
        json_file = requests.get(fa_tag + "/metadata/icons.json").json()

    icons = []
    max_len = 0

    for key, value in json_file.items():

        # sanitize name
        prefix = ''
        if key[0].isdigit() or key in cpp_keywords:
            prefix = '_'

        name = '{}{}'.format(prefix, key.replace('-', '_'))

        max_len = max(max_len, len(name))

        # get unicode
        code = '0x{}'.format(value["unicode"])

        # save
        icons.append((key, name, code))

    icons.sort(key=lambda e: e[1])

    with open('awesome.h', 'w') as file:
        file.write(('#ifndef AWESOME_H\n'
                    '#define AWESOME_H\n'
                    '\n'
                    '#include <QObject>\n'
                    '\n'
                    '/**\n'
                    ' * This file has been automatically generated.\n'
                    ' */\n'
                    '\n'
                    'namespace fa {\n'
                    '\n'
                    'static constexpr QLatin1String tag("' + fa_tag + '");\n'
                    '\n'
                    'static constexpr QLatin1String solid("solid");\n'
                    'static constexpr QLatin1String regular("regular");\n'
                    'static constexpr QLatin1String light("light");\n'
                    'static constexpr QLatin1String duotone("duotone");\n'
                    'static constexpr QLatin1String brands("brands");\n'
                    '\n'
                    'class v6 : public QObject {\n'
                    '    v6() = delete;\n'
                    '    Q_OBJECT;\n'
                    'public: enum codepoints {\n'))

        for key, name, code in icons:
            line = '    {name:<{len}} = {code},\n'.format(name=name, code=code, len=max_len)
            file.write(line)

        file.write(('};\n'
                    'Q_ENUM(codepoints)\n'
                    '};\n'
                    '\n'
                    '}\n'
                    '\n'
                    '#endif // AWESOME_H\n'))
        
if __name__ == '__main__':
    main()
