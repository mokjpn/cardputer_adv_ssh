#!/usr/bin/env python3
"""Package existing successful builds; never flash hardware or read credentials."""
import hashlib
import pathlib
import shutil
import subprocess
import sys
import zipfile

repo = pathlib.Path(__file__).resolve().parents[1]
out = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else pathlib.Path('/workspace/artifacts/cardtft')
out.mkdir(parents=True, exist_ok=True)
for variant, build in [('external', 'build-cardtft'), ('internal', 'build')]:
    for name in ['cardputer_adv_ssh.bin', 'bootloader/bootloader.bin', 'partition_table/partition-table.bin', 'flash_args']:
        source = repo / build / name
        if not source.is_file() or not source.stat().st_size:
            raise SystemExit(f'Missing build artifact: {source}')
        target = out / variant / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    shutil.copy2(repo / build / 'config/sdkconfig.h', out / variant / 'sdkconfig.h')
for name in ['README.md', 'LICENSE', 'dependencies.lock']:
    shutil.copy2(repo / name, out / name)
shutil.copytree(repo / 'docs', out / 'docs', dirs_exist_ok=True)
# Include the complete tracked + nonignored source tree, including uncommitted
# implementation changes. Dependencies are pinned by the lock file.
files = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=repo).split(b'\0')
with zipfile.ZipFile(out / 'source.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
    for raw in sorted(set(files)):
        if raw:
            name = raw.decode()
            archive.write(repo / name, 'cardputer_adv_ssh/' + name)
checksums = []
for file in sorted(out.rglob('*')):
    if file.is_file() and file.name != 'SHA256SUMS':
        checksums.append(hashlib.sha256(file.read_bytes()).hexdigest() + '  ' + file.relative_to(out).as_posix())
(out / 'SHA256SUMS').write_text('\n'.join(checksums) + '\n')
archive = shutil.make_archive(str(out), 'zip', out.parent, out.name)
print(archive)
