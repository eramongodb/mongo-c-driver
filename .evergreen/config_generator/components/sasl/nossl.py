from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_task import EvgTaskRef

from config_generator.etc.function import merge_defns
from config_generator.etc.compile import generate_compile_tasks

from config_generator.etc.sasl.compile import CompileCommon
from config_generator.etc.sasl.test import generate_test_tasks


SSL = 'nossl'
TAG = f'sasl-matrix-{SSL}'


# pylint: disable=line-too-long
# fmt: off
COMPILE_MATRIX = [
    ('ubuntu2004',        'gcc',         None,   ['off']),
    ('windows-vsCurrent', 'vs2017x64',   None,   ['off']),
    ('debian10',          'gcc',         None,   ['off']),
]

TEST_MATRIX = [
    ('ubuntu2004', 'gcc', None, 'off', ['noauth'], ['server', 'replica', 'sharded'], ['4.4', '5.0', '6.0', '7.0', '8.0', 'latest']),

    # Test 4.2 with Debian 10 since 4.2 does not ship on Ubuntu 20.04+.
    ('debian10',   'gcc', None, 'off', ['noauth'], ['server', 'replica', 'sharded'], ['4.2']), 
]
# fmt: on
# pylint: enable=line-too-long


class NoSSLCompileCommon(CompileCommon):
    ssl = 'OFF'


class SaslOffNoSSLCompile(NoSSLCompileCommon):
    name = 'sasl-off-nossl-compile'
    commands = NoSSLCompileCommon.compile_commands(sasl='OFF')


def functions():
    return merge_defns(
        SaslOffNoSSLCompile.defn(),
    )


def tasks():
    res = []

    SASL_TO_FUNC = {
        'off': SaslOffNoSSLCompile,
    }

    res += generate_compile_tasks(SSL, TAG, SASL_TO_FUNC, COMPILE_MATRIX)
    res += generate_test_tasks(SSL, TAG, TEST_MATRIX)

    return res


def variants():
    expansions = {}

    return [
        BuildVariant(
            name=TAG,
            display_name=TAG,
            tasks=[EvgTaskRef(name=f'.{TAG}')],
            expansions=expansions,
        ),
    ]
