"""Replace git symlinks that Windows checked out as plain text files with copies of their targets.

usage: python -I fix_symlinks.py <repo_root>   (walks the repo and all submodules)
"""
import os, shutil, subprocess, sys

root = os.path.abspath(sys.argv[1])
repos = [root]
out = subprocess.run(['git', '-C', root, 'submodule', 'foreach', '--recursive', '--quiet', 'echo $toplevel/$sm_path'],
                     capture_output=True, text=True, check=True).stdout
repos += [os.path.normpath(l.strip()) for l in out.splitlines() if l.strip()]
fixed = 0
for repo in repos:
    ls = subprocess.run(['git', '-C', repo, 'ls-files', '-s'], capture_output=True, text=True, check=True).stdout
    links = [l.split('\t', 1)[1] for l in ls.splitlines() if l.startswith('120000 ')]
    # Resolve in passes: a link may point at another link.
    pending = list(links)
    for _ in range(5):
        left = []
        for rel in pending:
            p = os.path.join(repo, rel)
            if os.path.isdir(p) or (os.path.isfile(p) and os.path.getsize(p) > 1024):
                continue  # already real content
            target = open(p, encoding='utf-8', errors='replace').read().strip()
            src = os.path.normpath(os.path.join(os.path.dirname(p), target))
            if not os.path.exists(src) or (src in [os.path.normpath(os.path.join(repo, x)) for x in pending] and os.path.getsize(src) < 256):
                left.append(rel)
                continue
            os.remove(p)
            if os.path.isdir(src):
                shutil.copytree(src, p)
            else:
                shutil.copyfile(src, p)
            fixed += 1
        pending = left
        if not pending:
            break
    for rel in pending:
        print('UNRESOLVED', repo, rel)
    if links:
        print(repo, len(links), 'symlinks')
print('fixed', fixed)
