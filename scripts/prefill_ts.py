#!/usr/bin/env python3
"""
Prefill Qt .ts files: copy <source> text into <translation> and mark as finished.
Usage: python3 scripts/prefill_ts.py i18n/
"""
import sys
import os
import xml.etree.ElementTree as ET

if len(sys.argv) < 2:
    print("Usage: prefill_ts.py <i18n-dir>")
    sys.exit(1)

i18n_dir = sys.argv[1]
if not os.path.isdir(i18n_dir):
    print(f"Directory not found: {i18n_dir}")
    sys.exit(2)

for fname in sorted(os.listdir(i18n_dir)):
    if not fname.endswith('.ts'):
        continue
    path = os.path.join(i18n_dir, fname)
    print(f"Prefilling {path}")
    try:
        tree = ET.parse(path)
        root = tree.getroot()
        # Qt TS namespace handling: usually no namespace, but be tolerant
        for context in root.findall('context'):
            for message in context.findall('message'):
                source = message.find('source')
                trans = message.find('translation')
                if source is None:
                    continue
                stext = source.text or ''
                if trans is None:
                    trans = ET.SubElement(message, 'translation')
                # Set translation text to source if empty
                ttext = trans.text or ''
                if ttext.strip() == '':
                    trans.text = stext
                # Mark as finished by removing type attribute or setting it to 'finished'
                if 'type' in trans.attrib:
                    try:
                        del trans.attrib['type']
                    except Exception:
                        trans.attrib['type'] = 'finished'
        tree.write(path, encoding='utf-8', xml_declaration=True)
    except Exception as e:
        print(f"Failed to process {path}: {e}")

print("Prefill complete.")
