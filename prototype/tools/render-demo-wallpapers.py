"""Render the bundled CC0 artwork to RGB565 frames. Requires Pillow and NumPy.

Run from the repository root, then run package-demo-wallpapers.cjs.
Outputs are staged under build/wallpaper-pack-v1; originals are preserved.
"""
from pathlib import Path
import json
import math
import hashlib
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / 'assets/wallpapers'
OUT = ROOT / 'build/wallpaper-pack-v1/rendered'
OUT.mkdir(parents=True, exist_ok=True)
PREVIEWS = ASSETS / 'previews'
PREVIEWS.mkdir(exist_ok=True)


def load(scene, filename):
    return Image.open(ASSETS / 'sources' / scene / filename).convert('RGBA')


def tile(layer, width, offset):
    result = Image.new('RGBA', (width, layer.height))
    x = -int(round(offset)) % layer.width - layer.width
    while x < width:
        result.alpha_composite(layer, (x, 0))
        x += layer.width
    return result


def portrait(canvas, center):
    # Preserve the original aspect ratio and use nearest-neighbour pixel scaling.
    scaled_width = round(canvas.width * 640 / canvas.height)
    canvas = canvas.resize((scaled_width, 640), Image.Resampling.NEAREST)
    # Callers provide the center in normalized coordinates.
    left = max(0, min(scaled_width - 360, round(center * scaled_width - 180)))
    return canvas.crop((left, 0, left + 360, 640)).convert('RGB')


city = [load('neon-city', name) for name in ['back.png', 'middle.png', 'foreground.png']]
mountains = [load('moonlit-mountains', name) for name in
             ['sky.png', 'far-clouds.png', 'near-clouds.png', 'far-mountains.png', 'mountains.png', 'trees.png']]
space = [load('pixel-planet', name) for name in ['background.png', 'stars-far.png', 'stars-near.png']]
planet = load('pixel-planet', 'planet.png')
assert planet.size == (77 * 96, 96)
assert all(im.size == (9 * 640, 360) for im in space)


def render(scene, index, count):
    phase = (index % count) / count
    movement = math.sin(2 * math.pi * phase)
    if scene == 'neon-city':
        canvas = Image.new('RGBA', (688, 272), '#171326')
        for layer, amplitude in zip(city, [2, 8, 20]):
            canvas.alpha_composite(tile(layer, 688, amplitude * movement))
        return portrait(canvas, 0.51)
    if scene == 'moonlit-mountains':
        canvas = Image.new('RGBA', (320, 240), '#352a4c')
        for layer, amplitude in zip(mountains, [0, 5, 9, 3, 7, 12]):
            canvas.alpha_composite(tile(layer, 320, amplitude * movement))
        return portrait(canvas, 0.50)
    canvas = Image.new('RGBA', (640, 360), '#0c0b21')
    bg_index = int(phase * 9)
    for strip, amplitude in zip(space, [0, 4, 9]):
        layer = strip.crop((bg_index * 640, 0, (bg_index + 1) * 640, 360))
        canvas.alpha_composite(tile(layer, 640, movement * amplitude))
    canvas = portrait(canvas, 0.5).convert('RGBA')
    p = int(phase * 77)
    globe = planet.crop((p * 96, 0, (p + 1) * 96, 96)).resize((288, 288), Image.Resampling.NEAREST)
    canvas.alpha_composite(globe, (36, 220))
    return canvas.convert('RGB')


report = []
for scene, count in [('neon-city', 160), ('moonlit-mountains', 160), ('pixel-planet', 154)]:
    target = OUT / (scene + '.rgb565')
    if target.exists():
        raise FileExistsError(target)
    previews, frame_hashes, deltas = [], [], []
    previous = first = last = None
    with target.open('xb') as stream:
        for i in range(count):
            im = render(scene, i, count)
            rgb = np.asarray(im, dtype=np.uint16)
            packed = ((rgb[:, :, 0] >> 3) << 11) | ((rgb[:, :, 1] >> 2) << 5) | (rgb[:, :, 2] >> 3)
            data = packed.astype('<u2').tobytes()
            stream.write(data)
            frame_hashes.append(hashlib.sha256(data).hexdigest())
            # Preview the RGB565-quantized output, not a higher-quality source.
            decoded = np.stack([((packed >> 11) * 255 + 15) // 31,
                                (((packed >> 5) & 63) * 255 + 31) // 63,
                                ((packed & 31) * 255 + 15) // 31], axis=2).astype(np.uint8)
            if first is None:
                first = decoded.copy()
            if previous is not None:
                deltas.append(float(np.abs(decoded.astype(np.int16) - previous).mean()))
            previous = decoded.astype(np.int16)
            last = decoded
            if i % 2 == 0:
                previews.append(Image.fromarray(decoded).resize((180, 320), Image.Resampling.NEAREST))
            if i == 0:
                Image.fromarray(decoded).save(OUT / (scene + '-first.png'))
    assert render(scene, 0, count).tobytes() == render(scene, count, count).tobytes()
    assert len(set(frame_hashes)) > 10, 'Animation has insufficient motion'
    previews[0].save(PREVIEWS / (scene + '.gif'), save_all=True, append_images=previews[1:],
                     duration=100, loop=0, optimize=False, disposal=2)
    boundary = float(np.abs(first.astype(np.int16) - last.astype(np.int16)).mean())
    assert boundary <= max(deltas) + 0.001, 'Loop boundary exceeds normal frame changes'
    report.append({'name': scene, 'width': 360, 'height': 640, 'fps': 20, 'frames': count,
                   'seconds': count / 20, 'uniqueFrames': len(set(frame_hashes)),
                   'loopBoundaryMeanAbsoluteDifference': boundary,
                   'maxAdjacentMeanAbsoluteDifference': max(deltas),
                   'rawSha256': hashlib.sha256(target.read_bytes()).hexdigest()})
    print(scene, count, 'frames', target.stat().st_size, 'bytes', flush=True)
(OUT / 'render-validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
