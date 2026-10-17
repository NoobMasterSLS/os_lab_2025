import os
import stat
import errno
import time
import argparse
from fuse import FUSE, FuseOSError, Operations
from database import Database

class SQLiteFS(Operations):
    def __init__(self, database):
        self.db = database
        self._ensure_root()

    def _ensure_root(self):
        if not self.db.get_file('/'):
            self.db.create_file('/', stat.S_IFDIR | 0o755, os.getuid(), os.getgid(), is_dir=True)


    def _get_file(self, path):
        f = self.db.get_file(path)
        if not f:
            raise FuseOSError(errno.ENOENT)
        return f

    def _full_path(self, path):
        return path if path == '/' else path.rstrip('/')


    #FUSE операции
    def getattr(self, path, fh=None):
        f = self._get_file(path)
        return {
            'st_mode': f['mode'],
            'st_nlink': f['nlink'],
            'st_uid': f['uid'],
            'st_gid': f['gid'],
            'st_size': f['size'],
            'st_atime': f['atime'],
            'st_mtime': f['mtime'],
            'st_ctime': f['ctime'],
        }

    def readdir(self, path, fh):
        path = self._full_path(path)
        self._get_file(path) 
        files = self.db.list_dir(path)
        return ['.', '..'] + files

    def create(self, path, mode, fi=None):
        path = self._full_path(path)
        self.db.create_file(path, stat.S_IFREG | (mode & 0o777), os.getuid(), os.getgid())
        return 0

    def write(self, path, data, offset, fh=None):
        path = self._full_path(path)
        return self.db.write_data(path, data, offset)

    def read(self, path, size, offset, fh=None):
        path = self._full_path(path)
        return self.db.read_data(path, size, offset)

    def unlink(self, path):
        path = self._full_path(path)
        self._get_file(path)
        self.db.delete_file(path)
        return 0

    def mkdir(self, path, mode):
        path = self._full_path(path)
        self.db.create_file(path, stat.S_IFDIR | (mode & 0o777), os.getuid(), os.getgid(), is_dir=True)
        return 0

    def rmdir(self, path):
        path = self._full_path(path)
        #проверка что папка пуста
        if self.db.list_dir(path):
            raise FuseOSError(errno.ENOTEMPTY)
        self.db.delete_file(path)
        return 0

    #доп операции (опционально)
    def truncate(self, path, length, fh=None):
        path = self._full_path(path)
        #читаем, обрезаем/расширяем, пишем
        data = self.db.read_data(path, length + 1, 0)
        if len(data) > length:
            data = data[:length]
        else:
            data = data + b'\x00' * (length - len(data))
        self.db.write_data(path, data, 0)
        return 0

    def rename(self, old, new):
        old = self._full_path(old)
        new = self._full_path(new)
        f = self._get_file(old)
        #создаём новую запись и удаляем старую
        self.db.create_file(new, f['mode'], f['uid'], f['gid'])
        #копируем данные
        data = self.db.read_data(old, f['size'], 0)
        self.db.write_data(new, data, 0)
        self.db.delete_file(old)
        return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('mountpoint')
    parser.add_argument('--database', default='fs.db')
    parser.add_argument('--foreground', action='store_true')
    args = parser.parse_args()

    db = Database(args.database)
    fs = SQLiteFS(db)

    FUSE(fs, args.mountpoint, foreground=args.foreground, nothreads=False)


if __name__ == '__main__':
    main()