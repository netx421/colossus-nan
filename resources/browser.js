// resources/browser.js
// COLOSSUS terminal-style renderer + MPV integration
//
// - Hides the original page, rebuilds a "links-style" view
// - Monospace amber theme
// - One linear list of links, with images next to them
// - ▶ badge to play YouTube/media links in mpv

(function () {
    'use strict';

    // Hide everything ASAP to prevent the "real" page from ever flashing
    try {
        // Don't hide Telehack; it needs to show its own xterm UI
        if (!isTelehackHost()) {
            document.documentElement.style.visibility = 'hidden';
        }
    } catch (e) { }

    // ───────────────────────────────────────────────
    //  Helpers
    // ───────────────────────────────────────────────

    function resolveUrl(href) {
        try {
            return new URL(href, window.location.href).toString();
        } catch {
            return href;
        }
    }

    function isYouTubeUrl(href) {
        if (!href) return false;
        let u;
        try {
            u = new URL(href, window.location.href);
        } catch {
            return false;
        }
        const host = u.hostname.toLowerCase();
        return host.includes('youtube.com') || host.includes('youtu.be');
    }

    function isMediaUrl(href) {
        if (!href) return false;
        const l = href.toLowerCase();
        return (
            l.endsWith('.mp4')  ||
            l.endsWith('.webm') ||
            l.endsWith('.mkv')  ||
            l.endsWith('.mov')  ||
            l.endsWith('.mp3')  ||
            l.endsWith('.ogg')  ||
            l.endsWith('.flac') ||
            l.endsWith('.m4a')
        );
    }

    function isPlayableUrl(href) {
        return isYouTubeUrl(href) || isMediaUrl(href);
    }

    function postToNative(url) {
        try {
            const h = window.webkit &&
                      window.webkit.messageHandlers &&
                      window.webkit.messageHandlers.mpvPlayer;
            if (h && typeof h.postMessage === 'function') {
                h.postMessage(url);
            }
        } catch (e) {
            console.error('mpvPlayer postMessage failed:', e);
        }
    }

    // NEW: optional Telehack → native xterm bridge
    function postToXterm(message) {
        try {
            const h = window.webkit &&
                      window.webkit.messageHandlers &&
                      window.webkit.messageHandlers.xtermLauncher;
            if (h && typeof h.postMessage === 'function') {
                h.postMessage(message);
            }
        } catch (e) {
            console.error('xtermLauncher postMessage failed:', e);
        }
    }

    function extractText(node) {
        if (!node) return '';
        const clone = node.cloneNode(true);
        // Remove scripts/styles/nav aside etc. from text extraction
        const kill = clone.querySelectorAll('script, style, nav, footer, header');
        kill.forEach(el => el.remove());
        let text = clone.textContent || '';
        text = text.replace(/\s+/g, ' ').trim();
        return text;
    }

    // NEW: host check for Telehack
    function isTelehackHost() {
        try {
            const host = window.location.hostname.toLowerCase();
            return host.includes('telehack.com');
        } catch (e) {
            return false;
        }
    }

    // NEW: Telehack-only amber styling for the web xterm
    function applyTelehackAmberTheme() {
        try {
            const de = document.documentElement;
            if (de) {
                de.style.backgroundColor = '#000000';
                de.style.color = '#ffbf40';
                de.style.visibility = 'visible';
            }
            if (document.body) {
                document.body.style.backgroundColor = '#000000';
                document.body.style.color = '#ffbf40';
            }

            if (!document.getElementById('telehack-amber-style')) {
                const style = document.createElement('style');
                style.id = 'telehack-amber-style';
                style.type = 'text/css';
                style.textContent = `
                    .xterm-rows {
                        color: #ffbf40 !important;
                        font-family: "Fira Code", "DejaVu Sans Mono", monospace !important;
                    }
                    .xterm-cursor-block {
                        background-color: #ffbf40 !important;
                        color: #000000 !important;
                    }
                `;
                (document.head || document.documentElement).appendChild(style);
            }
        } catch (e) {
            console.error('COLOSSUS Telehack amber error:', e);
        }
    }

    // ───────────────────────────────────────────────
    //  Retro theme CSS
    // ───────────────────────────────────────────────

    function injectCss() {
const css = `
/* ============================================================
    COLOSSUS CRT MONOCHROME THEME — v1.0
    White / Grey / Black, Retro Terminal Style
============================================================ */

/* ------------------------------------------------------------
    GLOBAL RESET / BASE LAYER
------------------------------------------------------------ */
html, body {
    margin: 0 !important;
    padding: 0 !important;
    background-color: #000000 !important;   /* pure black */
    color: #d0d0d0 !important;              /* soft light grey */

    font-family: "Fira Code", "DejaVu Sans Mono", monospace !-important;
    font-size: 15px !important;
    line-height: 1.42;

    /* subtle CRT bloom in white/grey */
    text-shadow:
        0 0 2px #c0c0c0,
        0 0 4px #e0e0e0,
        0 0 7px #ffffff;
}

* {
    border-radius: 0 !important;
    box-shadow: none !important;
}

/* ------------------------------------------------------------
    LINKS — WHITE CORE + GREY GLOW
------------------------------------------------------------ */
a {
    color: #ffffff !important;  /* crisp white */
    text-decoration: none !important;
    text-shadow:
        0 0 2px #ffffff,
        0 0 5px #d0d0d0,
        0 0 8px #f0f0f0;
}

a:hover {
    color: #f5f5f5 !important;
    text-shadow:
        0 0 3px #ffffff,
        0 0 7px #e0e0e0,
        0 0 10px #ffffff;
    text-decoration: underline !important;
}

/* ------------------------------------------------------------
    TERMINAL ROOT WRAPPER
------------------------------------------------------------ */
#colossus-terminal-root {
    padding: 1em 1.25em;
}

/* ------------------------------------------------------------
    HEADER
------------------------------------------------------------ */
#colossus-header {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    margin-bottom: 1em;
    border-bottom: 1px solid #404040;
    padding-bottom: 0.3em;
}

#colossus-header-title {
    background: #f5f5f5;    /* light label block */
    color: #000000;
    padding: 0 0.35em;
    font-weight: bold;
    text-shadow: none !important;
}

#colossus-header-url {
    font-size: 11px;
    color: #b0b0b0;
    text-shadow:
        0 0 2px #b0b0b0,
        0 0 4px #d0d0d0;
}

/* ------------------------------------------------------------
    SECTION TITLES
------------------------------------------------------------ */
.colossus-section-title {
    font-weight: bold;
    margin-top: 1em;
    margin-bottom: 0.25em;
    color: #f0f0f0;

    text-shadow:
        0 0 3px #e0e0e0,
        0 0 6px #ffffff;
}

/* ------------------------------------------------------------
    PARAGRAPHS
------------------------------------------------------------ */
.colossus-paragraph {
    margin: 0.22em 0;
    color: #d8d8d8;

    text-shadow:
        0 0 2px #b0b0b0,
        0 0 4px #e0e0e0;
}

/* ------------------------------------------------------------
    LINK ROWS (LINKS2-STYLE NAV)
------------------------------------------------------------ */
.colossus-link-row {
    display: flex;
    align-items: flex-start;
    gap: 0.6em;
    margin: 0.25em 0;
    padding: 0.15em 0.2em;
    cursor: pointer;
    border-left: 2px solid transparent;
}

.colossus-link-row:hover {
    background: rgba(255, 255, 255, 0.06);
    border-left: 2px solid #f0f0f0;
}

.colossus-link-index {
    min-width: 3ch;
    text-align: right;
    color: #c0c0c0;
    text-shadow: 0 0 3px #e0e0e0;
}

.colossus-link-main {
    flex: 1;
}

.colossus-link-text {
    color: #ffffff;
    text-shadow:
        0 0 3px #ffffff,
        0 0 6px #e0e0e0;
}

.colossus-link-url {
    font-size: 10px;
    margin-top: 0.12em;
    color: #a8a8a8;
    opacity: 0.95;
    text-shadow: 0 0 3px #d0d0d0;
}

/* ============================================================
    THUMBNAILS — PURE MONOCHROME CRT
============================================================ */
.colossus-thumbnail {
    max-height: 122px;
    max-width: 145px;
    object-fit: contain;

    /* CRT frame */
    background: #050505;
    border: 1px solid #ffffff;

    /* strict monochrome */
    image-rendering: pixelated;
    filter:
        grayscale(100%)
        brightness(0.95)
        contrast(125%)
        drop-shadow(0 0 4px #f0f0f0);
}

/* -------------------------------------------------
   UNIVERSAL AMBER TINT FOR ALL IMAGES/ICONS
---------------------------------------------------*/
img:not(.no-amber),
picture img:not(.no-amber),
svg image:not(.no-amber) {
    image-rendering: pixelated;
    filter:
        grayscale(100%)
        sepia(100%)
        hue-rotate(-15deg)
        saturate(250%)
        brightness(0.85)
        drop-shadow(0 0 5px #ffae00);
}

/* -------------------------------------------------
   INLINE IMAGES — SAME CRT STYLE AS THUMBNAILS
   (Used when injecting <img> into content flow)
---------------------------------------------------*/
.colossus-inline-image {
    max-width: 320px;
    max-height: 240px;
    object-fit: contain;

    /* Same CRT frame + tint as thumbnails */
    background: #050200;
    border: 1px solid #ffffff;
    image-rendering: pixelated;
    filter:
        grayscale(100%)
        sepia(100%)
        hue-rotate(-15deg)
        saturate(250%)
        brightness(0.85)
        drop-shadow(0 0 5px #ffae00);

    display: block;
    margin: 0.25em 0;
}



    /* -------------------------------------------------
        MPV Badge
    ---------------------------------------------------*/
    .colossus-mpv-icon {
        display: inline-block;
        margin-left: 0.45em;
        padding: 0 0.35em;
        border: 1px solid #ffffff;
        background: #110900;
        color: #ffffff;
        font-size: 0.75em;
        border-radius: 2px;
        text-shadow:
            0 0 4px #ffffff,
            0 0 8px #ffae00;
        transition: 0.12s ease-out;
    }

    .colossus-mpv-icon:hover {
        background: #ffffff;
        color: #000000;
        text-shadow: none;
    }

    /* -------------------------------------------------
        Footer
    ---------------------------------------------------*/
    #colossus-footer-hint {
        margin-top: 1.4em;
        font-size: 11px;
        color: #ffffff;
        opacity: 0.85;
        text-shadow: 0 0 4px #ffae00;
        padding-top: 0.4em;
        border-top: 1px solid #403000;
    }

    #colossus-original {
        display: none !important;
    }

`;

        const style = document.createElement('style');
        style.id = 'colossus-style';
        style.textContent = css;
        document.documentElement.appendChild(style);
    }

    // ───────────────────────────────────────────────
    //  Terminal-style rebuild
    // ───────────────────────────────────────────────

    function buildTerminalView() {
        if (!document.body) return;

        // Avoid double init
        if (document.getElementById('colossus-terminal-root')) return;

        // Move original content aside so we can mine it without displaying.
        const original = document.createElement('div');
        original.id = 'colossus-original';
        while (document.body.firstChild) {
            original.appendChild(document.body.firstChild);
        }
        original.style.display = 'none';
        document.body.appendChild(original);

        // Main terminal-style root
        const root = document.createElement('div');
        root.id = 'colossus-terminal-root';
        document.body.insertBefore(root, original);

        // Header
        const header = document.createElement('div');
        header.id = 'colossus-header';

        const titleSpan = document.createElement('span');
        titleSpan.id = 'colossus-header-title';
        titleSpan.textContent = document.title || '[No Title]';

        const urlSpan = document.createElement('span');
        urlSpan.id = 'colossus-header-url';
        urlSpan.textContent = window.location.href;

        header.appendChild(titleSpan);
        header.appendChild(urlSpan);

        root.appendChild(header);

        const content = document.createElement('div');
        content.id = 'colossus-content';
        root.appendChild(content);

        // ── Main text (rough)
    // ── Main content: text + inline images in document order
    const mainTextTitle = document.createElement('div');
    mainTextTitle.className = 'colossus-section-title';
//  mainTextTitle.textContent = 'Content';
//    content.appendChild(mainTextTitle);


        // ── Link list
        const linksTitle = document.createElement('div');
        linksTitle.className = 'colossus-section-title';
        linksTitle.textContent = 'Links';
        content.appendChild(linksTitle);

        const anchors = original.querySelectorAll('a[href]');
        let index = 1;

        anchors.forEach(a => {
            const href = a.getAttribute('href');
            if (!href) return;

            const absUrl = resolveUrl(href);

            // Extract visible text
            let text = a.textContent || '';
            text = text.replace(/\s+/g, ' ').trim();

            // If the link is just an image, use alt/src
            const img = a.querySelector('img');
            if (!text) {
                if (img && img.alt) {
                    text = img.alt;
                } else {
                    text = absUrl;
                }
            }

            const row = document.createElement('div');
            row.className = 'colossus-link-row';

            const indexSpan = document.createElement('span');
            indexSpan.className = 'colossus-link-index';
            indexSpan.textContent = String(index).padStart(2, ' ') + '.';

            const main = document.createElement('div');
            main.className = 'colossus-link-main';

            const textSpan = document.createElement('span');
            textSpan.className = 'colossus-link-text';

            const linkEl = document.createElement('a');
            linkEl.href = absUrl;
            linkEl.textContent = text;

            textSpan.appendChild(linkEl);
            main.appendChild(textSpan);

            // Small URL line under the text (for context)
            const urlLine = document.createElement('div');
            urlLine.className = 'colossus-link-url';
            urlLine.textContent = absUrl;
            main.appendChild(urlLine);

            // Thumbnail next to link, if there is an image
            let thumbWrapper = null;
            if (img && img.src) {
                thumbWrapper = document.createElement('div');
                const thumb = document.createElement('img');
                thumb.className = 'colossus-thumbnail';
                thumb.src = img.src;
                thumbWrapper.appendChild(thumb);
            }

            // MPV icon for playable URLs
            if (isPlayableUrl(absUrl)) {
                const mpvIcon = document.createElement('span');
                mpvIcon.className = 'colossus-mpv-icon';
                mpvIcon.textContent = '▶ mpv';

                mpvIcon.addEventListener('click', function (ev) {
                    ev.preventDefault();
                    ev.stopPropagation();
                    postToNative(absUrl);
                });

                main.appendChild(mpvIcon);
            }

            // Clicking anywhere on row follows the link
            row.addEventListener('click', function (ev) {
                // If they clicked the mpv icon, that handler already ran
                if (ev.target.closest('.colossus-mpv-icon')) return;
                window.location.href = absUrl;
            });

            // index on the far left, then thumbnail, then text
            row.appendChild(indexSpan);
            if (thumbWrapper) {
                row.appendChild(thumbWrapper);
            }
            row.appendChild(main);

            content.appendChild(row);
            index++;
        });

    // h1/h2/h3/p/li + img in DOM order
    const flowNodes = original.querySelectorAll('h1, h2, h3, p, li, img');
    let flowCount = 0;
    const MAX_FLOW_ITEMS = 120; // safety so we don't flood on huge sites

    flowNodes.forEach(node => {
        if (flowCount >= MAX_FLOW_ITEMS) return;

        if (node.tagName && node.tagName.toLowerCase() === 'img') {
            const src = node.src;
            if (!src) return;

            const wrap = document.createElement('div');
            wrap.className = 'colossus-paragraph';

            const img = document.createElement('img');
            img.src = src;
            img.className = 'colossus-inline-image';

            // Optional: a bit larger than the small thumbnails
            img.style.maxWidth = '320px';
            img.style.maxHeight = '240px';
            img.style.display = 'block';
            img.style.margin = '0.25em 0';

            wrap.appendChild(img);
            content.appendChild(wrap);
            flowCount++;
            return;
        }

        // Text nodes (h1/h2/h3/p/li)
        const txt = extractText(node);
        if (!txt) return;

        const div = document.createElement('div');
        div.className = 'colossus-paragraph';
        div.textContent = txt;
        content.appendChild(div);
        flowCount++;
    });


        // Footer hint
        const footer = document.createElement('div');
        footer.id = 'colossus-footer-hint';
        footer.textContent =
            'COLOSSUS SYSTEM ACTIVE // SECURITY CLEARANCE OMEGA // ALL CHANNELS MONITORED UNAUTHORIZED ACCESS PROHIBITED';
        content.appendChild(footer);
    }

    // ───────────────────────────────────────────────
    //  Init
    // ───────────────────────────────────────────────
function isNativeAmberHost() {
    try {
        const host = window.location.hostname.toLowerCase();
        return (
            host.includes('@x421') ||     // Telehack special case
            host.includes('https://searchtec.tech/krush')  ||     // ← add other domains here
            host.includes('levidia.ch')      ||     // ← add more
            false
        );
    } catch (e) {
        return false;
    }
}
    function init() {
        try {
            injectCss();

if (isTelehackHost()) {
    // Telehack: keep xterm behavior + amber + native xterm bridge
    applyTelehackAmberTheme();
    postToXterm('telehack');

} else if (isNativeAmberHost()) {
    // Other special sites: keep native layout but tint amber
    applyTelehackAmberTheme(); // ← reuse the same amber theme
    // No DOM rewrite, no COLOSSUS terminal layout

} else {
    // Everything else gets full COLOSSUS retro terminal mode
    buildTerminalView();
}


            // Show page (or Telehack xterm) now that we're ready
            document.documentElement.style.visibility = 'visible';
        } catch (e) {
            console.error('COLOSSUS browser.js init error:', e);
            try {
                document.documentElement.style.visibility = 'visible';
            } catch (_) { }
        }
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();

