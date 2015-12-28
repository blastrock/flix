import pytest
import os
from testex import *

TMP_FILE='/tmp/TEST_TESTEX'

@pytest.fixture
def testex(request):
    def fin():
        os.remove(TMP_FILE)
    request.addfinalizer(fin)

@pytest.mark.parametrize("content,nbsuccess,nbfail", [
    (b'[ALL TESTS RUN]', 0, 0),
    (b'[BEGIN TEST "test"]\n[END TEST]\n[ALL TESTS RUN]', 1, 0),
    (b'[BEGIN TEST "test"]\n[FAIL]\n[END TEST]\n[ALL TESTS RUN]', 0, 1),
    (b'[BEGIN TEST "test"]\n[END TEST]\n[BEGIN TEST "test2"]\n[FAIL]\n[END TEST]\n[ALL TESTS RUN]', 1, 1),
])
def test_run(testex, content, nbsuccess, nbfail):
    open(TMP_FILE, 'wb').write(content)
    lp = run_and_process(['cat', TMP_FILE])
    assert lp.is_finished()
    assert not lp.error
    assert len(lp.successed_tests) == nbsuccess
    assert len(lp.failed_tests) == nbfail

@pytest.mark.parametrize("content", [
    (b'[BEGIN TEST "test"]\n[BEGIN TEST "test2"]\n[END TEST]\n[END TEST]\n[ALL TESTS RUN]'),
    (b'[BEGIN TEST "test"]\n[END TEST]\n[BEGIN TEST "test"]\n[END TEST]\n[ALL TESTS RUN]'),
    (b'[BEGIN TEST "test"]\n[END TEST]\n'),
    (b'[BEGIN TEST "test"]\n[END TEST]\n[FAIL]\n[ALL TESTS RUN]\n'),
    (b'[BEGIN TEST "test"]\n[ALL TESTS RUN]'),
    (b'[BEGIN TEST "test"]\n[END TEST]\n[ALL TESTS RUN]\n[BEGIN TEST "test"]\n'),
    (b'[END TEST]\n[ALL TESTS RUN]\n'),
    (b''),
])
def test_incorrect(testex, content):
    open(TMP_FILE, 'wb').write(content)
    lp = run_and_process(['cat', TMP_FILE])
    assert not lp.is_finished()
