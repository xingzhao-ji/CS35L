from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(Path('/dev/full').exists(), 'requires /dev/full')
class BufferedOutputTests(unittest.TestCase):
    def test_failed_output_returns_failure(self):
        for directory in ('assign5', 'assign6/randall-final'):
            subprocess.run(['make', '-s'], cwd=ROOT / directory, check=True)
            binary = ROOT / directory / 'randall'
            for count in (8, 16384):
                with self.subTest(directory=directory, count=count, sink='stdout'):
                    with open('/dev/full', 'wb') as sink:
                        result = subprocess.run([str(binary), str(count)], stdout=sink,
                                                stderr=subprocess.PIPE, timeout=5)
                    self.assertNotEqual(result.returncode, 0)
                with self.subTest(directory=directory, count=count, sink='file'):
                    result = subprocess.run([str(binary), '-o', '/dev/full', str(count)],
                                            capture_output=True, timeout=5)
                    self.assertNotEqual(result.returncode, 0)

    def test_successful_output_preserves_length(self):
        for directory in ('assign5', 'assign6/randall-final'):
            subprocess.run(['make', '-s'], cwd=ROOT / directory, check=True)
            for count in (0, 8, 16384):
                with self.subTest(directory=directory, count=count):
                    result = subprocess.run([str(ROOT / directory / 'randall'), str(count)],
                                            capture_output=True, timeout=5)
                    self.assertEqual(result.returncode, 0)
                    self.assertEqual(len(result.stdout), count)


if __name__ == '__main__':
    unittest.main()
