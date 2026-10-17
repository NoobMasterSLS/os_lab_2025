import unittest
import tempfile
import os
import stat
from sqlitefs import SQLiteFS
from database import Database

class TestSQLiteFS(unittest.TestCase):
    def setUp(self):
        self.temp_db = tempfile.NamedTemporaryFile(delete=False)
        self.db = Database(self.temp_db.name)
        self.fs = SQLiteFS(self.db)

    def tearDown(self):
        os.unlink(self.temp_db.name)

    def test_root_exists(self):
        attr = self.fs.getattr('/')
        self.assertTrue(stat.S_ISDIR(attr['st_mode']))

    def test_create_file(self):
        self.fs.create('/test.txt', 0o644)
        attr = self.fs.getattr('/test.txt')
        self.assertTrue(stat.S_ISREG(attr['st_mode']))
        self.assertEqual(attr['st_size'], 0)

    def test_write_read_file(self):
        self.fs.create('/data.txt', 0o644)
        self.fs.write('/data.txt', b'Hello', 0)
        data = self.fs.read('/data.txt', 10, 0)
        self.assertEqual(data, b'Hello')
        attr = self.fs.getattr('/data.txt')
        self.assertEqual(attr['st_size'], 5)

    def test_mkdir_readdir(self):
        self.fs.mkdir('/dir1', 0o755)
        self.fs.create('/dir1/file1.txt', 0o644)
        files = self.fs.readdir('/dir1', None)
        self.assertIn('file1.txt', files)

    def test_unlink(self):
        self.fs.create('/todelete.txt', 0o644)
        self.fs.unlink('/todelete.txt')
        with self.assertRaises(Exception):
            self.fs.getattr('/todelete.txt')

if __name__ == '__main__':
    unittest.main()