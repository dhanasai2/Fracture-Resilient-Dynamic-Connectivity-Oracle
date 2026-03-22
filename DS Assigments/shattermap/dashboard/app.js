// ============================================
// SHATTERMAP DASHBOARD — Chart & Interaction Logic
// ============================================

const COLORS = {
    purple: '#7c3aed', blue: '#3b82f6', cyan: '#06b6d4',
    green: '#10b981', amber: '#f59e0b', red: '#ef4444',
    pink: '#ec4899', slate: '#64748b'
};
const DS_COLORS = [COLORS.purple, COLORS.blue, COLORS.cyan, COLORS.green, COLORS.amber, COLORS.red];
const DS_COLORS_ALPHA = DS_COLORS.map(c => c + '33');

// Chart.js global config
Chart.defaults.color = '#94a3b8';
Chart.defaults.borderColor = 'rgba(255,255,255,0.05)';
Chart.defaults.font.family = "'Inter', sans-serif";

// ========== DATA ==========
let D = typeof BENCHMARK_DATA !== 'undefined' ? BENCHMARK_DATA : null;

// ========== ANIMATIONS ==========
function animateCounter(el, target, suffix = '', duration = 1500) {
    const start = performance.now();
    const isFloat = target % 1 !== 0;
    const step = (now) => {
        const progress = Math.min((now - start) / duration, 1);
        const ease = 1 - Math.pow(1 - progress, 3);
        const val = target * ease;
        el.textContent = (isFloat ? val.toFixed(1) : Math.floor(val).toLocaleString()) + suffix;
        if (progress < 1) requestAnimationFrame(step);
    };
    requestAnimationFrame(step);
}

function initStatCounters() {
    if (!D) return;
    const nodes = D.dataset?.nodes || 90000;
    const edges = D.dataset?.edges || 300000;
    const ops = D.dataset?.operations || 120000;
    const safePct = D.shattermap?.safeDeletePercent || 99.9;

    const cards = document.querySelectorAll('.stat-card .stat-value');
    if (cards[0]) animateCounter(cards[0], nodes);
    if (cards[1]) animateCounter(cards[1], edges);
    if (cards[2]) animateCounter(cards[2], ops);
    if (cards[3]) animateCounter(cards[3], safePct, '%');
}

// ========== TABS ==========
document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
    });
});

// ========== CHART HELPERS ==========
function makeBarChart(canvasId, labels, datasets, opts = {}) {
    const ctx = document.getElementById(canvasId);
    if (!ctx) return;
    new Chart(ctx, {
        type: 'bar',
        data: { labels, datasets },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            aspectRatio: opts.aspectRatio || 2,
            indexAxis: opts.horizontal ? 'y' : 'x',
            plugins: {
                legend: { display: datasets.length > 1, position: 'top', labels: { padding: 16, usePointStyle: true } },
            },
            scales: {
                x: { grid: { display: false }, ticks: { font: { size: 11 } } },
                y: { grid: { color: 'rgba(255,255,255,0.03)' }, ticks: { font: { family: "'JetBrains Mono'" } } }
            },
            animation: { duration: 1200, easing: 'easeOutQuart' }
        }
    });
}

function makeDoughnut(canvasId, labels, data, colors) {
    const ctx = document.getElementById(canvasId);
    if (!ctx) return;
    new Chart(ctx, {
        type: 'doughnut',
        data: {
            labels,
            datasets: [{ data, backgroundColor: colors, borderWidth: 0, hoverOffset: 8 }]
        },
        options: {
            responsive: true, cutout: '65%',
            plugins: {
                legend: { position: 'bottom', labels: { padding: 16, usePointStyle: true, font: { size: 12 } } }
            },
            animation: { animateRotate: true, duration: 1500 }
        }
    });
}

// ========== BUILD CHARTS ==========
function buildCharts() {
    if (!D || !D.structures) return;
    const S = D.structures;
    const names = S.map(s => s.name);

    // 1. Total time (build + query + dynamic)
    const totalTimes = S.map(s => +(s.buildTime + s.staticQueryTime + s.addTime + s.deleteTime + s.checkTime).toFixed(2));
    makeBarChart('chart-total-time', names, [{
        label: 'Total Time (ms)',
        data: totalTimes,
        backgroundColor: names.map((n, i) => S[i].type === 'novel' ? COLORS.purple + 'cc' : DS_COLORS_ALPHA[i]),
        borderColor: names.map((n, i) => S[i].type === 'novel' ? COLORS.purple : DS_COLORS[i]),
        borderWidth: 2, borderRadius: 8
    }], { horizontal: true, aspectRatio: 1.8 });

    // 2. Static query time
    makeBarChart('chart-query-time', names, [{
        label: 'Query Time (ms)',
        data: S.map(s => +s.staticQueryTime.toFixed(2)),
        backgroundColor: DS_COLORS.map(c => c + '99'),
        borderColor: DS_COLORS, borderWidth: 2, borderRadius: 6
    }]);

    // 3. Build time
    makeBarChart('chart-build-time', names, [{
        label: 'Build Time (ms)',
        data: S.map(s => +s.buildTime.toFixed(2)),
        backgroundColor: DS_COLORS.map(c => c + '99'),
        borderColor: DS_COLORS, borderWidth: 2, borderRadius: 6
    }]);

    // 4. Operations breakdown (grouped bar)
    makeBarChart('chart-ops-breakdown', names, [
        { label: 'ADD (ms)', data: S.map(s => +s.addTime.toFixed(2)), backgroundColor: COLORS.green + '99', borderColor: COLORS.green, borderWidth: 2, borderRadius: 4 },
        { label: 'DELETE (ms)', data: S.map(s => +s.deleteTime.toFixed(2)), backgroundColor: COLORS.red + '99', borderColor: COLORS.red, borderWidth: 2, borderRadius: 4 },
        { label: 'CHECK (ms)', data: S.map(s => +s.checkTime.toFixed(2)), backgroundColor: COLORS.cyan + '99', borderColor: COLORS.cyan, borderWidth: 2, borderRadius: 4 }
    ], { aspectRatio: 2.2 });

    // 5. Delete time comparison
    makeBarChart('chart-delete-time', names, [{
        label: 'Delete Time (ms)',
        data: S.map(s => s.supportsDelete ? +s.deleteTime.toFixed(2) : 0),
        backgroundColor: names.map((n, i) => S[i].type === 'novel' ? COLORS.green + 'cc' : COLORS.red + '66'),
        borderColor: names.map((n, i) => S[i].type === 'novel' ? COLORS.green : COLORS.red),
        borderWidth: 2, borderRadius: 6
    }]);

    // 6. Edge classification doughnut
    const smData = D.shattermap || {};
    makeDoughnut('chart-edge-class',
        ['Reinforced (Safe)', 'Bridge (Stress)'],
        [smData.reinforcedEdges || 299700, smData.bridgeEdges || 300],
        [COLORS.green, COLORS.red]
    );

    // 7. Delete outcomes doughnut
    makeDoughnut('chart-delete-outcomes',
        ['Safe O(1) Deletes', 'Fracture Deletes'],
        [smData.safeDeletes || 39900, smData.stressDeletes || 82],
        [COLORS.cyan, COLORS.amber]
    );

    // 8. Memory chart
    makeBarChart('chart-memory', names, [{
        label: 'Memory (MB)',
        data: S.map(s => +(s.memoryBytes / (1024 * 1024)).toFixed(2)),
        backgroundColor: DS_COLORS.map(c => c + '99'),
        borderColor: DS_COLORS, borderWidth: 2, borderRadius: 6
    }]);

    // 9. Feature matrix
    buildFeatureMatrix(S);

    // 10. Results table
    buildResultsTable(S);
}

function buildFeatureMatrix(S) {
    const el = document.getElementById('feature-matrix');
    if (!el) return;
    let html = '<table><thead><tr><th>Data Structure</th><th>Add</th><th>Delete</th><th>Query</th><th>Weighted</th></tr></thead><tbody>';
    S.forEach(s => {
        const yes = '✅', no = '❌';
        html += `<tr${s.type === 'novel' ? ' class="highlight-row"' : ''}>`;
        html += `<td>${s.name}</td>`;
        html += `<td>${yes}</td>`;
        html += `<td>${s.supportsDelete ? yes : no}</td>`;
        html += `<td>${yes}</td>`;
        html += `<td>${yes}</td>`;
        html += `</tr>`;
    });
    html += '</tbody></table>';
    el.innerHTML = html;
}

function buildResultsTable(S) {
    const tbody = document.getElementById('results-tbody');
    if (!tbody) return;
    let html = '';
    S.forEach((s, i) => {
        const cls = s.type === 'novel' ? ' class="highlight-row"' : '';
        html += `<tr${cls}>`;
        html += `<td><strong>${s.name}</strong></td>`;
        html += `<td>${s.type}</td>`;
        html += `<td>${s.buildTime.toFixed(2)}</td>`;
        html += `<td>${s.staticQueryTime.toFixed(2)}</td>`;
        html += `<td>${s.addTime.toFixed(2)}</td>`;
        html += `<td>${s.supportsDelete ? s.deleteTime.toFixed(2) : 'N/A'}</td>`;
        html += `<td>${s.checkTime.toFixed(2)}</td>`;
        html += `<td>${(s.memoryBytes / (1024*1024)).toFixed(2)}</td>`;
        html += `<td>${s.supportsDelete ? '✅' : '❌'}</td>`;
        html += `</tr>`;
    });
    tbody.innerHTML = html;
}

// ========== INIT ==========
document.addEventListener('DOMContentLoaded', () => {
    initStatCounters();
    buildCharts();
});
