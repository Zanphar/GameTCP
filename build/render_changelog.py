import html
import re
from datetime import date

lines = open('CHANGES.md', encoding='utf-8').read().replace('\r\n', '\n').split('\n')
RELEASE = re.compile(r'^## \[([0-9.]+)\] - (\d{4}-\d{2}-\d{2})$')
UL = re.compile(r'^[-*] ')
OL = re.compile(r'^\d+\. ')


def inline(text):
    text = html.escape(text, quote=False)
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)',
                  lambda m: '<a href="%s">%s</a>' % (m.group(2), m.group(1)), text)
    text = re.sub(r'`([^`]+)`', r'<code>\1</code>', text)
    text = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', text)
    return text


def cells(row):
    return [part.strip() for part in row.strip().strip('|').split('|')]


out = []
para = []
open_release = False


def flush():
    if para:
        out.append('<p>' + inline(' '.join(x.strip() for x in para)) + '</p>')
        para.clear()


def close_release():
    global open_release
    if open_release:
        out.append('</section>')
        open_release = False


i = 0
while i < len(lines):
    raw = lines[i]
    s = raw.strip()

    if not s:
        flush()
        i += 1
        continue

    if s.startswith('# '):
        flush()
        i += 1
        continue

    if s.startswith('## '):
        flush()
        close_release()
        m = RELEASE.match(s)
        if m:
            version, iso = m.groups()
            anchor = 'v' + version.replace('.', '-')
            human = date.fromisoformat(iso).strftime('%B %d, %Y')
            out.append('<section class="release" id="' + anchor + '">')
            out.append('<div class="release-head"><h2>' + version + '</h2><time datetime="' +
                       iso + '">' + human + '</time></div>')
            open_release = True
        else:
            out.append('<h2 class="section-title">' + inline(s[3:]) + '</h2>')
        i += 1
        continue

    if s.startswith('### '):
        flush()
        out.append('<h3>' + inline(s[4:]) + '</h3>')
        i += 1
        continue

    if s.startswith('|'):
        flush()
        rows = []
        while i < len(lines) and lines[i].strip().startswith('|'):
            rows.append(cells(lines[i]))
            i += 1
        if not rows:
            continue
        body = rows[1:]
        if body and all(re.fullmatch(r':?-{2,}:?', c.replace(' ', '')) for c in body[0]):
            body = body[1:]
        table = ['<div class="table-wrap"><table><thead><tr>']
        table += ['<th>' + inline(c) + '</th>' for c in rows[0]]
        table.append('</tr></thead><tbody>')
        for row in body:
            table.append('<tr>' + ''.join('<td>' + inline(c) + '</td>' for c in row) + '</tr>')
        table.append('</tbody></table></div>')
        out.append(''.join(table))
        continue

    if UL.match(s):
        flush()
        items = []
        while i < len(lines):
            cur = lines[i]
            st = cur.strip()
            if UL.match(st):
                items.append(st[2:].strip())
                i += 1
            elif items and st and cur.startswith(('  ', '\t')) and not st.startswith('#'):
                items[-1] += ' ' + st
                i += 1
            else:
                break
        out.append('<ul>' + ''.join('<li>' + inline(x) + '</li>' for x in items) + '</ul>')
        continue

    if OL.match(s):
        flush()
        items = []
        while i < len(lines):
            cur = lines[i]
            st = cur.strip()
            m = re.match(r'^\d+\. (.*)$', st)
            if m:
                items.append(m.group(1).strip())
                i += 1
            elif items and st and cur.startswith(('  ', '\t')) and not st.startswith('#'):
                items[-1] += ' ' + st
                i += 1
            else:
                break
        out.append('<ol>' + ''.join('<li>' + inline(x) + '</li>' for x in items) + '</ol>')
        continue

    para.append(s)
    i += 1

flush()
close_release()

TEMPLATE = '''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Changelog - GamerTCP 4</title>
<meta name="description" content="Release history for GamerTCP 4, the native C++ Windows network optimization toolkit.">
<meta name="color-scheme" content="dark">
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Crect width='32' height='32' rx='8' fill='%235ad1ff'/%3E%3Cpath d='M8 21l6-10 4 6 6-9' stroke='%23070b14' stroke-width='3' fill='none' stroke-linecap='round' stroke-linejoin='round'/%3E%3C/svg%3E">
<style>
  :root{--bg:#060a12;--bg-2:#0a101c;--panel:rgba(17,26,43,.72);--line:rgba(255,255,255,.09);--text:#e9eefb;--muted:#9aa8c4;--dim:#6f7d99;--accent:#5ad1ff;--accent-2:#8b7bff}
  *{box-sizing:border-box}
  html{scroll-behavior:smooth}
  body{margin:0;color:var(--text);background:radial-gradient(900px 520px at 82% -8%,rgba(139,123,255,.22),transparent 62%),radial-gradient(760px 480px at 8% 0,rgba(90,209,255,.16),transparent 60%),linear-gradient(180deg,var(--bg-2),var(--bg) 42rem);font:16px/1.6 ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;-webkit-font-smoothing:antialiased}
  a{color:var(--accent);text-decoration:none}
  a:hover{text-decoration:underline}
  .wrap{width:min(100% - 2.5rem,1000px);margin-inline:auto}
  .site-header{position:sticky;top:0;z-index:40;backdrop-filter:blur(14px);background:rgba(6,10,18,.72);border-bottom:1px solid var(--line)}
  .header-inner{display:flex;align-items:center;gap:1.3rem;height:64px}
  .brand{display:flex;align-items:center;gap:.6rem;color:var(--text);font-weight:700;letter-spacing:-.01em;font-size:1.02rem}
  .brand:hover{text-decoration:none}
  .brand svg{width:26px;height:26px;border-radius:8px;flex:none}
  .brand span{color:var(--accent)}
  .site-header nav{display:flex;gap:1.25rem;margin-left:auto;font-size:.92rem}
  .site-header nav a{color:var(--muted)}
  .site-header nav a:hover{color:var(--text);text-decoration:none}
  .btn{display:inline-flex;align-items:center;justify-content:center;padding:.62rem .95rem;border-radius:10px;background:linear-gradient(135deg,var(--accent),#3aa8f0 45%,var(--accent-2));color:#04121c;font-weight:650;text-decoration:none;font-size:.9rem;border:1px solid transparent}
  .btn:hover{text-decoration:none;filter:brightness(1.05)}
  .page-head{padding:3.6rem 0 1rem}
  .eyebrow{margin:0 0 .7rem;color:var(--accent);font-size:.78rem;font-weight:650;letter-spacing:.12em;text-transform:uppercase}
  h1{margin:0;font-size:clamp(2rem,4vw,2.9rem);line-height:1.1;letter-spacing:-.03em}
  .lede{margin:1rem 0 0;color:var(--muted);max-width:46rem;font-size:1.02rem}
  .changelog{padding:1.3rem 0 4.5rem}
  .changelog > p{max-width:56rem;color:var(--muted);font-size:.95rem}
  .changelog > ul,.changelog > ol{max-width:56rem;padding-left:1.3rem;color:var(--muted);font-size:.94rem}
  .changelog > ul li,.changelog > ol li{margin:.35rem 0}
  .release{max-width:56rem;margin:0 0 1.1rem;padding:1.5rem 1.6rem;border:1px solid var(--line);border-radius:16px;background:var(--panel);scroll-margin-top:5rem}
  .release-head{display:flex;align-items:baseline;flex-wrap:wrap;gap:.35rem .9rem;border-bottom:1px solid var(--line);padding-bottom:.75rem;margin-bottom:.9rem}
  .release-head h2{margin:0;font-size:1.35rem;color:var(--accent);letter-spacing:-.02em}
  .release-head time{color:var(--dim);font-size:.9rem}
  .release h3{margin:1.25rem 0 .4rem;color:var(--dim);font-size:.78rem;text-transform:uppercase;letter-spacing:.1em}
  .release h3:first-of-type{margin-top:0}
  .release p{color:var(--muted);font-size:.95rem}
  .release ul{padding-left:1.2rem;color:var(--muted);font-size:.94rem}
  .release li{margin:.38rem 0}
  .release code,.changelog code{font-family:ui-monospace,"Cascadia Code",Consolas,monospace;font-size:.86em;color:#c6d2e8;background:rgba(255,255,255,.06);padding:.08em .32em;border-radius:5px;overflow-wrap:anywhere}
  .section-title{max-width:56rem;margin:2.8rem 0 .8rem;color:var(--text);font-size:1.35rem;letter-spacing:-.02em;scroll-margin-top:5rem}
  .table-wrap{max-width:56rem;margin:1rem 0;overflow-x:auto;border:1px solid var(--line);border-radius:10px}
  table{border-collapse:collapse;width:100%;font-size:.9rem}
  th,td{padding:.65rem .8rem;text-align:left;vertical-align:top;border-bottom:1px solid var(--line);color:var(--muted)}
  th{background:rgba(255,255,255,.04);color:var(--text);font-weight:600}
  tr:last-child td{border-bottom:0}
  footer{border-top:1px solid var(--line);padding:2.2rem 0 2.8rem;color:var(--dim);font-size:.86rem}
  .footer-inner{display:flex;flex-wrap:wrap;gap:1rem 2rem;align-items:center;justify-content:space-between}
  .footer-inner a{color:var(--muted)}
  @media(max-width:760px){.site-header nav{display:none}.page-head{padding:2.8rem 0 .8rem}.release{padding:1.2rem 1.1rem}.changelog{padding-bottom:3.2rem}}
</style>
</head>
<body>
<header class="site-header">
  <div class="wrap header-inner">
    <a class="brand" href="index.html" aria-label="GamerTCP 4 home"><svg viewBox="0 0 32 32" aria-hidden="true"><rect width="32" height="32" rx="8" fill="#5ad1ff"/><path d="M8 21l6-10 4 6 6-9" stroke="#070b14" stroke-width="3" fill="none" stroke-linecap="round" stroke-linejoin="round"/></svg>GamerTCP <span>4</span></a>
    <nav aria-label="Page sections"><a href="index.html">Home</a><a href="index.html#releases">Release notes</a><a href="index.html#download">Download</a></nav>
    <a class="btn" href="index.html#download">Get GamerTCP 4</a>
  </div>
</header>
<main>
  <section class="page-head"><div class="wrap">
    <p class="eyebrow">Changelog</p>
    <h1>Release history</h1>
    <p class="lede">Every tagged GamerTCP 4 release, newest first. Prefer a file? <a href="downloads/CHANGES.md" download>Download CHANGES.md</a>.</p>
  </div></section>
  <section class="changelog"><div class="wrap">
@@CONTENT@@
  </div></section>
</main>
<footer><div class="wrap footer-inner">
  <p style="margin:0">GamerTCP 4 · BSD 3-Clause · Tweak set derived from GameTCP3 © 2022 Charles McDonald</p>
  <p style="margin:0"><a href="index.html">Home</a> · <a href="downloads/CHANGES.md" download>CHANGES.md</a> · <a href="index.html#download">Download</a></p>
</div></footer>
</body>
</html>
'''

page = TEMPLATE.replace('@@CONTENT@@', '\n'.join(out))
with open('site/changelog.html', 'w', encoding='utf-8', newline='\n') as handle:
    handle.write(page)
print('wrote site/changelog.html', len(page), 'bytes;', page.count('class="release"'), 'releases')
