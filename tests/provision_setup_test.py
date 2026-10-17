"""Run with python3 -m unittest discover -s tests -p '*_test.py'."""
import pathlib
import re
import shutil
import stat
import subprocess
import tempfile
import unittest


class ProvisionSetupTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        (self.root / "scripts").mkdir()
        self.script = self.root / "scripts/provision_setup.py"
        shutil.copy(pathlib.Path(__file__).resolve().parents[1] / "scripts/provision_setup.py",
                    self.script)

    def run_script(self):
        return subprocess.run(["python3", str(self.script)], capture_output=True, text=True)

    def test_secrets_are_private_distinct_and_not_printed(self):
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        config = (self.root / "sdkconfig").read_text()
        passwords = re.findall(r'PASSWORD="([a-f0-9]{32})"', config)
        self.assertEqual(len(passwords), 2)
        self.assertNotEqual(*passwords)
        for password in passwords:
            self.assertNotIn(password, result.stdout + result.stderr)
            self.assertIn(password, (self.root / "setup-credentials.local.txt").read_text())
        for name in ("sdkconfig", "setup-credentials.local.txt"):
            self.assertEqual(stat.S_IMODE((self.root / name).stat().st_mode), 0o600)
        self.assertNotEqual(self.run_script().returncode, 0)
        self.assertEqual((self.root / "sdkconfig").read_text(), config)

    def test_existing_configuration_is_preserved(self):
        original = 'CONFIG_EXISTING=y\nCONFIG_FOLLOWUP_WIFI_AP_PASSWORD=""\n'
        (self.root / "sdkconfig").write_text(original)
        self.assertEqual(self.run_script().returncode, 0)
        config = (self.root / "sdkconfig").read_text()
        self.assertIn("CONFIG_EXISTING=y", config)
        self.assertEqual(config.count("CONFIG_FOLLOWUP_WIFI_AP_PASSWORD="), 1)

    def test_existing_credentials_file_is_not_overwritten(self):
        file = self.root / "setup-credentials.local.txt"
        file.write_text("keep me")
        self.assertNotEqual(self.run_script().returncode, 0)
        self.assertEqual(file.read_text(), "keep me")
        self.assertFalse((self.root / "sdkconfig").exists())
