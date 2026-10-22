/**
 * ResQRelief - Disaster Relief and Resource Management System
 * Frontend Application Controller
 * Connects to C++ Crow Backend REST API
 */

const API_BASE = ''; // Relative path since frontend is served directly by C++ Crow server

// Application State
let appState = {
  disasters: [],
  resources: [],
  requests: [],
  distributions: [],
  stats: null,
  activeView: 'dashboard',
  deleteTarget: null // { type: 'disaster'|'resource'|'request', id: number, name: string }
};

// Initialize Application on DOM Ready
document.addEventListener('DOMContentLoaded', () => {
  initClock();
  setupNavigation();
  setupEventListeners();
  checkBackendHealth();
  loadAllData();
});

// Live Clock
function initClock() {
  const clockEl = document.getElementById('current-time');
  const update = () => {
    const now = new Date();
    clockEl.textContent = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
  };
  update();
  setInterval(update, 1000);
}

// Navigation & View Switching
function setupNavigation() {
  const navItems = document.querySelectorAll('.nav-item');
  navItems.forEach(item => {
    item.addEventListener('click', (e) => {
      e.preventDefault();
      const view = item.getAttribute('data-view');
      switchView(view);
    });
  });

  // Mobile sidebar toggle
  const openBtn = document.getElementById('open-sidebar-btn');
  const closeBtn = document.getElementById('close-sidebar-btn');
  const sidebar = document.getElementById('sidebar');

  if (openBtn) openBtn.addEventListener('click', () => sidebar.classList.add('mobile-open'));
  if (closeBtn) closeBtn.addEventListener('click', () => sidebar.classList.remove('mobile-open'));
}

function switchView(viewName) {
  appState.activeView = viewName;

  // Update navigation items
  document.querySelectorAll('.nav-item').forEach(el => {
    if (el.getAttribute('data-view') === viewName) {
      el.classList.add('active');
    } else {
      el.classList.remove('active');
    }
  });

  // Update view sections
  document.querySelectorAll('.content-view').forEach(view => {
    view.classList.remove('active');
  });

  const activeSection = document.getElementById(`view-${viewName}`);
  if (activeSection) {
    activeSection.classList.add('active');
  }

  // Update title & subtitle
  const titleEl = document.getElementById('page-title');
  const subtitleEl = document.getElementById('page-subtitle');
  const titles = {
    dashboard: { t: 'Disaster Relief Operations Dashboard', s: 'Real-time incident tracking, relief logistics, and inventory allocation' },
    disasters: { t: 'Disaster Incidents Management', s: 'Record, filter, update, and coordinate emergency incident response' },
    inventory: { t: 'Emergency Resource Inventory', s: 'Warehouse stockpiles, food, water, medical supplies, and relief equipment' },
    requests: { t: 'Community Relief Requests', s: 'Field requests from camps, clinics, and relief coordinators' },
    distribution: { t: 'Resource Allocation & Distribution', s: 'Stock deduction engine, convoy dispatching, and consignment logging' },
    reports: { t: 'Operations Audit & Statistical Reports', s: 'Official disaster logistics report with print-ready summary' }
  };

  if (titles[viewName]) {
    titleEl.textContent = titles[viewName].t;
    subtitleEl.textContent = titles[viewName].s;
  }

  // Close mobile sidebar if open
  document.getElementById('sidebar').classList.remove('mobile-open');

  // Specific view refresh
  if (viewName === 'distribution') {
    populateAllocationDropdowns();
  } else if (viewName === 'reports') {
    renderReportsView();
  }
}

// Event Listeners for Filters & Modals
function setupEventListeners() {
  // Disasters search and filters
  const disSearch = document.getElementById('disaster-search-input');
  const disType = document.getElementById('disaster-type-filter');
  const disSev = document.getElementById('disaster-severity-filter');
  if (disSearch) disSearch.addEventListener('input', renderDisastersTable);
  if (disType) disType.addEventListener('change', renderDisastersTable);
  if (disSev) disSev.addEventListener('change', renderDisastersTable);

  // Inventory search and filter
  const invSearch = document.getElementById('inventory-search-input');
  const invCat = document.getElementById('inventory-category-filter');
  if (invSearch) invSearch.addEventListener('input', renderInventoryTable);
  if (invCat) invCat.addEventListener('change', renderInventoryTable);

  // Requests search and filter
  const reqSearch = document.getElementById('request-search-input');
  const reqStatus = document.getElementById('request-status-filter');
  const reqUrg = document.getElementById('request-urgency-filter');
  if (reqSearch) reqSearch.addEventListener('input', renderRequestsTable);
  if (reqStatus) reqStatus.addEventListener('change', renderRequestsTable);
  if (reqUrg) reqUrg.addEventListener('change', renderRequestsTable);

  // Confirm delete button
  const confirmDeleteBtn = document.getElementById('confirm-delete-btn');
  if (confirmDeleteBtn) {
    confirmDeleteBtn.addEventListener('click', executeDeletion);
  }
}

// Check Backend Health
async function checkBackendHealth() {
  const offlineBanner = document.getElementById('offline-banner');
  const statusIndicator = document.getElementById('status-indicator');
  const statusLabel = document.getElementById('status-label');

  try {
    const res = await fetch(`${API_BASE}/api/stats`, { method: 'GET' });
    if (res.ok) {
      offlineBanner.classList.add('hidden');
      statusIndicator.className = 'status-indicator online';
      statusLabel.textContent = 'C++ Server Online';
      return true;
    }
  } catch (err) {
    offlineBanner.classList.remove('hidden');
    statusIndicator.className = 'status-indicator offline';
    statusLabel.textContent = 'C++ Backend Offline';
    return false;
  }
}

// Data Loading Orchestrator
async function loadAllData() {
  try {
    await Promise.all([
      fetchStats(),
      fetchDisasters(),
      fetchResources(),
      fetchRequests(),
      fetchDistributions()
    ]);
    renderAllViews();
  } catch (err) {
    console.error('Error fetching data from C++ Crow API:', err);
    showToast('Failed to connect with C++ backend server.', 'error');
  }
}

// ========================================================
// API CALLS
// ========================================================
async function fetchStats() {
  const res = await fetch(`${API_BASE}/api/stats`);
  if (!res.ok) throw new Error('Failed to fetch stats');
  appState.stats = await res.json();
}

async function fetchDisasters() {
  const res = await fetch(`${API_BASE}/api/disasters`);
  if (!res.ok) throw new Error('Failed to fetch disasters');
  const data = await res.json();
  appState.disasters = data.disasters || [];
}

async function fetchResources() {
  const res = await fetch(`${API_BASE}/api/resources`);
  if (!res.ok) throw new Error('Failed to fetch resources');
  const data = await res.json();
  appState.resources = data.resources || [];
}

async function fetchRequests() {
  const res = await fetch(`${API_BASE}/api/requests`);
  if (!res.ok) throw new Error('Failed to fetch requests');
  const data = await res.json();
  appState.requests = data.requests || [];
}

async function fetchDistributions() {
  const res = await fetch(`${API_BASE}/api/distributions`);
  if (!res.ok) throw new Error('Failed to fetch distributions');
  const data = await res.json();
  appState.distributions = data.distributions || [];
}

// ========================================================
// RENDERERS
// ========================================================
function renderAllViews() {
  renderDashboard();
  renderDisastersTable();
  renderInventoryTable();
  renderRequestsTable();
  renderDistributionHistory();
  updateSidebarBadges();
}

function updateSidebarBadges() {
  document.getElementById('disaster-count-badge').textContent = appState.disasters.length;
  document.getElementById('inventory-count-badge').textContent = appState.resources.length;
  
  const pendingCount = appState.requests.filter(r => r.status === 'Pending').length;
  document.getElementById('request-pending-badge').textContent = pendingCount;
}

// Dashboard View
function renderDashboard() {
  const s = appState.stats;
  if (!s) return;

  document.getElementById('card-total-disasters').textContent = s.totalDisasters;
  document.getElementById('card-active-disasters').textContent = `${s.activeDisasters} Active / ${s.criticalDisasters} Critical`;

  document.getElementById('card-total-stock').textContent = s.totalStockItems.toLocaleString();
  document.getElementById('card-resource-types').textContent = `${s.totalResourceTypes} distinct categories`;

  document.getElementById('card-pending-requests').textContent = s.pendingRequests;
  document.getElementById('card-total-requests').textContent = `${s.totalRequests} Total Requests`;

  document.getElementById('card-total-distributions').textContent = s.totalDistributions;
  document.getElementById('card-items-distributed').textContent = `${s.totalItemsDistributed.toLocaleString()} items dispatched`;

  // Low stock warning banner
  const lowStockBox = document.getElementById('low-stock-alert');
  if (s.lowStockAlerts > 0) {
    lowStockBox.classList.remove('hidden');
  } else {
    lowStockBox.classList.add('hidden');
  }

  // Recent disasters table on dashboard
  const disTable = document.getElementById('dashboard-disasters-table');
  const recentDis = appState.disasters.slice(0, 5);
  if (recentDis.length === 0) {
    disTable.innerHTML = `<tr><td colspan="6" class="text-center py-4 text-muted">No disasters recorded yet.</td></tr>`;
  } else {
    disTable.innerHTML = recentDis.map(d => `
      <tr>
        <td><strong>${escapeHtml(d.name)}</strong></td>
        <td>${escapeHtml(d.type)}</td>
        <td><i class="fa-solid fa-location-dot text-muted"></i> ${escapeHtml(d.location)}</td>
        <td><span class="badge ${getSeverityBadge(d.severity)}">${escapeHtml(d.severity)}</span></td>
        <td><span class="badge ${getStatusBadge(d.status)}">${escapeHtml(d.status)}</span></td>
        <td>
          <button class="btn btn-sm btn-outline" onclick="openEditDisasterModal(${d.id})">Edit</button>
        </td>
      </tr>
    `).join('');
  }

  // Recent distributions table on dashboard
  const distTable = document.getElementById('dashboard-distributions-table');
  const recentDist = appState.distributions.slice(0, 5);
  if (recentDist.length === 0) {
    distTable.innerHTML = `<tr><td colspan="5" class="text-center py-4 text-muted">No distributions logged yet.</td></tr>`;
  } else {
    distTable.innerHTML = recentDist.map(dist => `
      <tr>
        <td><strong>${escapeHtml(dist.resourceName)}</strong></td>
        <td>${escapeHtml(dist.destination)}</td>
        <td><strong>${dist.quantity.toLocaleString()}</strong></td>
        <td>${escapeHtml(dist.date)}</td>
        <td><span class="badge badge-completed">${escapeHtml(dist.status)}</span></td>
      </tr>
    `).join('');
  }

  // Category Stock Bars
  renderCategoryBars();
}

function renderCategoryBars() {
  const container = document.getElementById('dashboard-category-bars');
  if (!container) return;

  const catMap = {};
  let totalStock = 0;
  appState.resources.forEach(r => {
    catMap[r.category] = (catMap[r.category] || 0) + r.quantity;
    totalStock += r.quantity;
  });

  if (totalStock === 0) {
    container.innerHTML = `<p class="text-muted text-sm">No inventory items recorded.</p>`;
    return;
  }

  container.innerHTML = Object.entries(catMap).map(([cat, qty]) => {
    const pct = Math.round((qty / totalStock) * 100);
    return `
      <div class="cat-progress-item">
        <div class="cat-progress-header">
          <span>${escapeHtml(cat)}</span>
          <span><strong>${qty.toLocaleString()}</strong> (${pct}%)</span>
        </div>
        <div class="cat-progress-track">
          <div class="cat-progress-fill" style="width: ${pct}%"></div>
        </div>
      </div>
    `;
  }).join('');
}

// Disasters Table
function renderDisastersTable() {
  const tbody = document.getElementById('disasters-table-body');
  if (!tbody) return;

  const search = (document.getElementById('disaster-search-input')?.value || '').toLowerCase();
  const typeFilter = document.getElementById('disaster-type-filter')?.value || '';
  const sevFilter = document.getElementById('disaster-severity-filter')?.value || '';

  const filtered = appState.disasters.filter(d => {
    const matchesSearch = d.name.toLowerCase().includes(search) || d.location.toLowerCase().includes(search);
    const matchesType = !typeFilter || d.type === typeFilter;
    const matchesSev = !sevFilter || d.severity === sevFilter;
    return matchesSearch && matchesType && matchesSev;
  });

  if (filtered.length === 0) {
    tbody.innerHTML = `<tr><td colspan="9" class="text-center py-4 text-muted">No matching disaster incidents found.</td></tr>`;
    return;
  }

  tbody.innerHTML = filtered.map(d => `
    <tr>
      <td>#${d.id}</td>
      <td><strong>${escapeHtml(d.name)}</strong></td>
      <td>${escapeHtml(d.type)}</td>
      <td><i class="fa-solid fa-location-dot text-muted"></i> ${escapeHtml(d.location)}</td>
      <td><span class="badge ${getSeverityBadge(d.severity)}">${escapeHtml(d.severity)}</span></td>
      <td>${escapeHtml(d.date)}</td>
      <td><span class="badge ${getStatusBadge(d.status)}">${escapeHtml(d.status)}</span></td>
      <td><small class="text-muted">${escapeHtml(truncate(d.description || '', 45))}</small></td>
      <td class="text-right">
        <button class="btn btn-sm btn-outline" onclick="openEditDisasterModal(${d.id})" title="Edit Disaster">
          <i class="fa-solid fa-pen-to-square"></i>
        </button>
        <button class="btn btn-sm btn-danger" onclick="confirmDelete('disaster', ${d.id}, '${escapeHtml(d.name)}')" title="Delete Disaster">
          <i class="fa-solid fa-trash"></i>
        </button>
      </td>
    </tr>
  `).join('');
}

// Inventory Table
function renderInventoryTable() {
  const tbody = document.getElementById('inventory-table-body');
  if (!tbody) return;

  const search = (document.getElementById('inventory-search-input')?.value || '').toLowerCase();
  const catFilter = document.getElementById('inventory-category-filter')?.value || '';

  const filtered = appState.resources.filter(r => {
    const matchesSearch = r.name.toLowerCase().includes(search) || 
                          r.category.toLowerCase().includes(search) || 
                          r.storageLocation.toLowerCase().includes(search);
    const matchesCat = !catFilter || r.category === catFilter;
    return matchesSearch && matchesCat;
  });

  if (filtered.length === 0) {
    tbody.innerHTML = `<tr><td colspan="8" class="text-center py-4 text-muted">No inventory resources found.</td></tr>`;
    return;
  }

  tbody.innerHTML = filtered.map(r => {
    let stockBadge = '<span class="badge badge-completed">Adequate</span>';
    if (r.quantity <= 0) {
      stockBadge = '<span class="badge badge-critical">Out of Stock</span>';
    } else if (r.quantity < 50) {
      stockBadge = '<span class="badge badge-high">Low Stock</span>';
    } else if (r.quantity < 200) {
      stockBadge = '<span class="badge badge-medium">Moderate</span>';
    }

    return `
      <tr>
        <td>#${r.id}</td>
        <td><strong>${escapeHtml(r.name)}</strong></td>
        <td><span class="badge badge-low">${escapeHtml(r.category)}</span></td>
        <td><strong class="text-primary">${r.quantity.toLocaleString()}</strong></td>
        <td>${escapeHtml(r.unit)}</td>
        <td><i class="fa-solid fa-warehouse text-muted"></i> ${escapeHtml(r.storageLocation)}</td>
        <td>${stockBadge}</td>
        <td class="text-right">
          <button class="btn btn-sm btn-outline" onclick="openEditResourceModal(${r.id})" title="Edit Resource">
            <i class="fa-solid fa-pen-to-square"></i>
          </button>
          <button class="btn btn-sm btn-danger" onclick="confirmDelete('resource', ${r.id}, '${escapeHtml(r.name)}')" title="Delete Resource">
            <i class="fa-solid fa-trash"></i>
          </button>
        </td>
      </tr>
    `;
  }).join('');
}

// Requests Table
function renderRequestsTable() {
  const tbody = document.getElementById('requests-table-body');
  if (!tbody) return;

  const search = (document.getElementById('request-search-input')?.value || '').toLowerCase();
  const statusFilter = document.getElementById('request-status-filter')?.value || '';
  const urgFilter = document.getElementById('request-urgency-filter')?.value || '';

  const filtered = appState.requests.filter(r => {
    const matchesSearch = r.requesterName.toLowerCase().includes(search) || 
                          r.requiredResource.toLowerCase().includes(search) || 
                          r.location.toLowerCase().includes(search);
    const matchesStatus = !statusFilter || r.status === statusFilter;
    const matchesUrg = !urgFilter || r.urgency === urgFilter;
    return matchesSearch && matchesStatus && matchesUrg;
  });

  if (filtered.length === 0) {
    tbody.innerHTML = `<tr><td colspan="9" class="text-center py-4 text-muted">No relief requests found.</td></tr>`;
    return;
  }

  tbody.innerHTML = filtered.map(req => `
    <tr>
      <td>#${req.id}</td>
      <td><strong>${escapeHtml(req.requesterName)}</strong></td>
      <td><i class="fa-solid fa-location-dot text-muted"></i> ${escapeHtml(req.location)}</td>
      <td><strong>${escapeHtml(req.requiredResource)}</strong></td>
      <td><strong>${req.quantity.toLocaleString()}</strong></td>
      <td><span class="badge ${getSeverityBadge(req.urgency)}">${escapeHtml(req.urgency)}</span></td>
      <td><span class="badge ${getRequestStatusBadge(req.status)}">${escapeHtml(req.status)}</span></td>
      <td><small class="text-muted">${escapeHtml(req.contactInfo || 'N/A')}</small></td>
      <td class="text-right">
        ${req.status === 'Pending' ? `
          <button class="btn btn-sm btn-primary" onclick="updateRequestStatus(${req.id}, 'Approved')" title="Approve Request">Approve</button>
          <button class="btn btn-sm btn-outline" onclick="updateRequestStatus(${req.id}, 'Rejected')" title="Reject Request">Reject</button>
        ` : ''}
        ${req.status === 'Approved' ? `
          <button class="btn btn-sm btn-secondary" onclick="openAllocateForRequest(${req.id})">Dispatch Aid</button>
          <button class="btn btn-sm btn-outline" onclick="updateRequestStatus(${req.id}, 'Completed')">Done</button>
        ` : ''}
        ${req.status === 'In Progress' ? `
          <button class="btn btn-sm btn-primary" onclick="updateRequestStatus(${req.id}, 'Completed')">Complete</button>
        ` : ''}
        <button class="btn btn-sm btn-danger" onclick="confirmDelete('request', ${req.id}, 'Request from ${escapeHtml(req.requesterName)}')" title="Delete Request">
          <i class="fa-solid fa-trash"></i>
        </button>
      </td>
    </tr>
  `).join('');
}

// Distribution History Table
function renderDistributionHistory() {
  const tbody = document.getElementById('distribution-history-table');
  if (!tbody) return;

  if (appState.distributions.length === 0) {
    tbody.innerHTML = `<tr><td colspan="6" class="text-center py-4 text-muted">No distribution records recorded yet.</td></tr>`;
    return;
  }

  tbody.innerHTML = appState.distributions.map(d => `
    <tr>
      <td>#${d.id}</td>
      <td><strong>${escapeHtml(d.resourceName)}</strong> (${d.quantity.toLocaleString()})</td>
      <td>${escapeHtml(d.disasterName)}</td>
      <td>${escapeHtml(d.destination)}</td>
      <td>${escapeHtml(d.date)}</td>
      <td><span class="badge badge-completed">${escapeHtml(d.status)}</span></td>
    </tr>
  `).join('');
}

// Reports View Renderer
async function renderReportsView() {
  try {
    const res = await fetch(`${API_BASE}/api/reports`);
    if (!res.ok) throw new Error('Failed to fetch detailed report');
    const rep = await res.json();

    document.getElementById('report-gen-date').textContent = new Date().toLocaleString();
    document.getElementById('rep-total-disasters').textContent = rep.disasters.length;

    const activeCount = rep.disasters.filter(d => d.status === 'Active').length;
    document.getElementById('rep-active-disasters').textContent = activeCount;

    const totalStock = rep.resources.reduce((sum, r) => sum + r.quantity, 0);
    document.getElementById('rep-total-stock').textContent = totalStock.toLocaleString();

    const pendingReq = rep.requests.filter(r => r.status === 'Pending').length;
    document.getElementById('rep-pending-requests').textContent = pendingReq;

    const dispatchedQty = rep.distributions.reduce((sum, d) => sum + d.quantity, 0);
    document.getElementById('rep-units-dispatched').textContent = dispatchedQty.toLocaleString();

    // Disasters Table in Report
    const disTbody = document.getElementById('report-disasters-tbody');
    disTbody.innerHTML = rep.disasters.map(d => `
      <tr>
        <td>#${d.id}</td>
        <td><strong>${escapeHtml(d.name)}</strong></td>
        <td>${escapeHtml(d.type)}</td>
        <td>${escapeHtml(d.location)}</td>
        <td>${escapeHtml(d.severity)}</td>
        <td>${escapeHtml(d.date)}</td>
        <td>${escapeHtml(d.status)}</td>
      </tr>
    `).join('');

    // Resources Table in Report
    const resTbody = document.getElementById('report-resources-tbody');
    resTbody.innerHTML = rep.resources.map(r => `
      <tr>
        <td><strong>${escapeHtml(r.name)}</strong></td>
        <td>${escapeHtml(r.category)}</td>
        <td><strong>${r.quantity.toLocaleString()}</strong></td>
        <td>${escapeHtml(r.unit)}</td>
        <td>${escapeHtml(r.storageLocation)}</td>
      </tr>
    `).join('');

    // Distributions Table in Report
    const distTbody = document.getElementById('report-distributions-tbody');
    distTbody.innerHTML = rep.distributions.map(d => `
      <tr>
        <td>#${d.id}</td>
        <td>${escapeHtml(d.resourceName)}</td>
        <td><strong>${d.quantity.toLocaleString()}</strong></td>
        <td>${escapeHtml(d.disasterName)}</td>
        <td>${escapeHtml(d.destination)}</td>
        <td>${escapeHtml(d.date)}</td>
        <td>${escapeHtml(d.status)}</td>
      </tr>
    `).join('');

  } catch (err) {
    showToast('Failed to generate report: ' + err.message, 'error');
  }
}

// ========================================================
// ALLOCATION LOGIC & DROPDOWNS
// ========================================================
function populateAllocationDropdowns() {
  const resSelect = document.getElementById('alloc-resource-select');
  const disSelect = document.getElementById('alloc-disaster-select');
  const reqSelect = document.getElementById('alloc-request-select');

  if (!resSelect || !disSelect) return;

  // Resources
  resSelect.innerHTML = `<option value="">-- Choose resource --</option>` +
    appState.resources.map(r => `
      <option value="${r.id}" data-stock="${r.quantity}" data-unit="${escapeHtml(r.unit)}" ${r.quantity <= 0 ? 'disabled' : ''}>
        ${escapeHtml(r.name)} [Stock: ${r.quantity.toLocaleString()} ${escapeHtml(r.unit)}]
      </option>
    `).join('');

  // Disasters
  disSelect.innerHTML = `<option value="">-- Choose disaster incident --</option>` +
    appState.disasters.map(d => `
      <option value="${d.id}" data-location="${escapeHtml(d.location)}">
        #${d.id} - ${escapeHtml(d.name)} (${escapeHtml(d.location)}) [${escapeHtml(d.status)}]
      </option>
    `).join('');

  // Requests
  reqSelect.innerHTML = `<option value="0">-- Direct Disaster Allocation (No Request) --</option>` +
    appState.requests
      .filter(r => r.status === 'Pending' || r.status === 'Approved' || r.status === 'In Progress')
      .map(r => `
        <option value="${r.id}" data-location="${escapeHtml(r.location)}" data-qty="${r.quantity}" data-resource="${escapeHtml(r.requiredResource)}">
          #${r.id} - Request from ${escapeHtml(r.requesterName)} (${r.quantity} ${escapeHtml(r.requiredResource)} for ${escapeHtml(r.location)})
        </option>
      `).join('');
}

function handleAllocResourceChange() {
  const sel = document.getElementById('alloc-resource-select');
  const opt = sel.options[sel.selectedIndex];
  const feedback = document.getElementById('alloc-stock-feedback');

  if (!opt || !opt.value) {
    feedback.innerHTML = `Available Stock: <strong>0 units</strong>`;
    return;
  }

  const stock = parseInt(opt.getAttribute('data-stock'), 10) || 0;
  const unit = opt.getAttribute('data-unit') || 'units';

  feedback.innerHTML = `Available Stock: <strong class="text-primary">${stock.toLocaleString()} ${unit}</strong>`;
  validateAllocQuantity();
}

function handleAllocDisasterChange() {
  const sel = document.getElementById('alloc-disaster-select');
  const opt = sel.options[sel.selectedIndex];
  const destInput = document.getElementById('alloc-destination-input');

  if (opt && opt.value && !destInput.value) {
    destInput.value = opt.getAttribute('data-location') || '';
  }
}

function validateAllocQuantity() {
  const sel = document.getElementById('alloc-resource-select');
  const opt = sel.options[sel.selectedIndex];
  const qtyInput = document.getElementById('alloc-quantity-input');
  const warning = document.getElementById('alloc-qty-warning');
  const submitBtn = document.getElementById('btn-submit-alloc');

  if (!opt || !opt.value) return;

  const stock = parseInt(opt.getAttribute('data-stock'), 10) || 0;
  const qty = parseInt(qtyInput.value, 10) || 0;

  if (qty > stock) {
    warning.textContent = `Requested quantity (${qty}) exceeds available stock (${stock})!`;
    warning.classList.remove('hidden');
    submitBtn.disabled = true;
  } else {
    warning.classList.add('hidden');
    submitBtn.disabled = false;
  }
}

async function handleAllocationSubmit(e) {
  e.preventDefault();

  const resourceId = parseInt(document.getElementById('alloc-resource-select').value, 10);
  const disasterId = parseInt(document.getElementById('alloc-disaster-select').value, 10);
  const quantity = parseInt(document.getElementById('alloc-quantity-input').value, 10);
  const destination = document.getElementById('alloc-destination-input').value.trim();
  const requestId = parseInt(document.getElementById('alloc-request-select').value, 10) || 0;
  const notes = document.getElementById('alloc-notes-input').value.trim();

  if (!resourceId || !disasterId || !quantity || quantity <= 0) {
    showToast('Please fill all required allocation fields.', 'warning');
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/distributions`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ resourceId, disasterId, quantity, destination, requestId, notes })
    });

    const data = await res.json();
    if (!res.ok) {
      throw new Error(data.error || 'Allocation rejected by C++ backend');
    }

    showToast(data.message || 'Resource dispatched successfully!', 'success');
    document.getElementById('distribution-form').reset();
    document.getElementById('alloc-stock-feedback').innerHTML = `Available Stock: <strong>0 units</strong>`;

    // Refresh state
    await loadAllData();
    populateAllocationDropdowns();

  } catch (err) {
    showToast(err.message, 'error');
  }
}

function openAllocateModal() {
  switchView('distribution');
}

function openAllocateForRequest(reqId) {
  switchView('distribution');
  const req = appState.requests.find(r => r.id === reqId);
  if (!req) return;

  // Try to match required resource
  const matchedRes = appState.resources.find(r => r.name.toLowerCase().includes(req.requiredResource.toLowerCase()));
  if (matchedRes) {
    const resSel = document.getElementById('alloc-resource-select');
    resSel.value = matchedRes.id;
    handleAllocResourceChange();
  }

  // Pre-fill destination and quantity
  document.getElementById('alloc-quantity-input').value = req.quantity;
  document.getElementById('alloc-destination-input').value = req.location;

  const reqSel = document.getElementById('alloc-request-select');
  reqSel.value = req.id;
  validateAllocQuantity();
}

// ========================================================
// DISASTERS CRUD (MODAL & FORMS)
// ========================================================
function openNewDisasterModal() {
  document.getElementById('disaster-form').reset();
  document.getElementById('disaster-id-input').value = '';
  document.getElementById('disaster-modal-title').innerHTML = `<i class="fa-solid fa-burst text-danger"></i> Report New Disaster`;
  document.getElementById('disaster-submit-btn').textContent = 'Save Disaster';
  document.getElementById('disaster-date').value = new Date().toISOString().split('T')[0];
  document.getElementById('disaster-modal').classList.remove('hidden');
}

function openEditDisasterModal(id) {
  const d = appState.disasters.find(item => item.id === id);
  if (!d) return;

  document.getElementById('disaster-id-input').value = d.id;
  document.getElementById('disaster-name').value = d.name;
  document.getElementById('disaster-type').value = d.type;
  document.getElementById('disaster-severity').value = d.severity;
  document.getElementById('disaster-location').value = d.location;
  document.getElementById('disaster-date').value = d.date;
  document.getElementById('disaster-status').value = d.status;
  document.getElementById('disaster-desc').value = d.description || '';

  document.getElementById('disaster-modal-title').innerHTML = `<i class="fa-solid fa-pen-to-square text-primary"></i> Edit Disaster #${d.id}`;
  document.getElementById('disaster-submit-btn').textContent = 'Update Disaster';
  document.getElementById('disaster-modal').classList.remove('hidden');
}

function closeDisasterModal() {
  document.getElementById('disaster-modal').classList.add('hidden');
}

async function handleDisasterSubmit(e) {
  e.preventDefault();

  const id = document.getElementById('disaster-id-input').value;
  const payload = {
    name: document.getElementById('disaster-name').value.trim(),
    type: document.getElementById('disaster-type').value,
    location: document.getElementById('disaster-location').value.trim(),
    severity: document.getElementById('disaster-severity').value,
    date: document.getElementById('disaster-date').value,
    status: document.getElementById('disaster-status').value,
    description: document.getElementById('disaster-desc').value.trim()
  };

  try {
    let res;
    if (id) {
      // Update
      res = await fetch(`${API_BASE}/api/disasters/${id}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
    } else {
      // Create
      res = await fetch(`${API_BASE}/api/disasters`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
    }

    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to save disaster incident');

    showToast(id ? 'Disaster details updated.' : 'New disaster recorded.', 'success');
    closeDisasterModal();
    await loadAllData();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ========================================================
// RESOURCES CRUD
// ========================================================
function openNewResourceModal() {
  document.getElementById('resource-form').reset();
  document.getElementById('resource-id-input').value = '';
  document.getElementById('resource-modal-title').innerHTML = `<i class="fa-solid fa-boxes-stacked text-primary"></i> Add Inventory Resource`;
  document.getElementById('resource-submit-btn').textContent = 'Save Resource';
  document.getElementById('resource-modal').classList.remove('hidden');
}

function openEditResourceModal(id) {
  const r = appState.resources.find(item => item.id === id);
  if (!r) return;

  document.getElementById('resource-id-input').value = r.id;
  document.getElementById('resource-name').value = r.name;
  document.getElementById('resource-category').value = r.category;
  document.getElementById('resource-unit').value = r.unit;
  document.getElementById('resource-quantity').value = r.quantity;
  document.getElementById('resource-storage').value = r.storageLocation;

  document.getElementById('resource-modal-title').innerHTML = `<i class="fa-solid fa-pen-to-square text-primary"></i> Edit Resource #${r.id}`;
  document.getElementById('resource-submit-btn').textContent = 'Update Resource';
  document.getElementById('resource-modal').classList.remove('hidden');
}

function closeResourceModal() {
  document.getElementById('resource-modal').classList.add('hidden');
}

async function handleResourceSubmit(e) {
  e.preventDefault();

  const id = document.getElementById('resource-id-input').value;
  const quantity = parseInt(document.getElementById('resource-quantity').value, 10);

  if (quantity < 0) {
    showToast('Resource quantity cannot be negative.', 'error');
    return;
  }

  const payload = {
    name: document.getElementById('resource-name').value.trim(),
    category: document.getElementById('resource-category').value,
    quantity: quantity,
    unit: document.getElementById('resource-unit').value.trim(),
    storageLocation: document.getElementById('resource-storage').value.trim()
  };

  try {
    let res;
    if (id) {
      res = await fetch(`${API_BASE}/api/resources/${id}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
    } else {
      res = await fetch(`${API_BASE}/api/resources`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
    }

    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to save resource record');

    showToast(id ? 'Resource inventory updated.' : 'New resource added to warehouse.', 'success');
    closeResourceModal();
    await loadAllData();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ========================================================
// RELIEF REQUESTS CRUD
// ========================================================
function openNewRequestModal() {
  document.getElementById('request-form').reset();
  document.getElementById('request-modal').classList.remove('hidden');
}

function closeRequestModal() {
  document.getElementById('request-modal').classList.add('hidden');
}

async function handleRequestSubmit(e) {
  e.preventDefault();

  const payload = {
    requesterName: document.getElementById('req-requester').value.trim(),
    requiredResource: document.getElementById('req-resource').value.trim(),
    quantity: parseInt(document.getElementById('req-quantity').value, 10),
    location: document.getElementById('req-location').value.trim(),
    urgency: document.getElementById('req-urgency').value,
    status: 'Pending',
    contactInfo: document.getElementById('req-contact').value.trim()
  };

  try {
    const res = await fetch(`${API_BASE}/api/requests`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to submit request');

    showToast('Relief request submitted successfully.', 'success');
    closeRequestModal();
    await loadAllData();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function updateRequestStatus(id, newStatus) {
  try {
    const res = await fetch(`${API_BASE}/api/requests/${id}/status`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ status: newStatus })
    });

    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to update request status');

    showToast(`Request #${id} marked as ${newStatus}.`, 'success');
    await loadAllData();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ========================================================
// DELETION CONFIRMATION & EXECUTION
// ========================================================
function confirmDelete(type, id, name) {
  appState.deleteTarget = { type, id, name };
  const msgEl = document.getElementById('delete-modal-msg');
  msgEl.innerHTML = `Are you sure you want to permanently delete <strong>"${escapeHtml(name)}"</strong> (#${id})?`;
  document.getElementById('delete-modal').classList.remove('hidden');
}

function closeDeleteModal() {
  document.getElementById('delete-modal').classList.add('hidden');
  appState.deleteTarget = null;
}

async function executeDeletion() {
  if (!appState.deleteTarget) return;

  const { type, id } = appState.deleteTarget;
  let endpoint = '';
  if (type === 'disaster') endpoint = `/api/disasters/${id}`;
  else if (type === 'resource') endpoint = `/api/resources/${id}`;
  else if (type === 'request') endpoint = `/api/requests/${id}`;

  try {
    const res = await fetch(`${API_BASE}${endpoint}`, { method: 'DELETE' });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to delete record');

    showToast('Record deleted successfully.', 'success');
    closeDeleteModal();
    await loadAllData();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// Re-seed Sample Data
async function confirmResetData() {
  if (!confirm('Re-seed database with realistic sample disasters and resources? This resets current records.')) {
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/reset-sample-data`, { method: 'POST' });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to re-seed data');

    showToast('Database reset and re-seeded with sample data.', 'success');
    await loadAllData();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ========================================================
// UTILITY FUNCTIONS
// ========================================================
function getSeverityBadge(sev) {
  switch (sev) {
    case 'Critical': return 'badge-critical';
    case 'High': return 'badge-high';
    case 'Medium': return 'badge-medium';
    case 'Low': return 'badge-low';
    case 'Urgent': return 'badge-critical';
    default: return 'badge-low';
  }
}

function getStatusBadge(status) {
  switch (status) {
    case 'Active': return 'badge-active';
    case 'Contained': return 'badge-contained';
    case 'Resolved': return 'badge-resolved';
    default: return 'badge-resolved';
  }
}

function getRequestStatusBadge(status) {
  switch (status) {
    case 'Pending': return 'badge-pending';
    case 'Approved': return 'badge-approved';
    case 'In Progress': return 'badge-progress';
    case 'Completed': return 'badge-completed';
    case 'Rejected': return 'badge-rejected';
    default: return 'badge-pending';
  }
}

function escapeHtml(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

function truncate(str, len) {
  if (!str) return '';
  return str.length > len ? str.substring(0, len) + '...' : str;
}

// Toast Notifications
function showToast(message, type = 'info') {
  const container = document.getElementById('toast-container');
  if (!container) return;

  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;

  let icon = 'fa-circle-info';
  if (type === 'success') icon = 'fa-circle-check';
  if (type === 'error') icon = 'fa-triangle-exclamation';
  if (type === 'warning') icon = 'fa-circle-exclamation';

  toast.innerHTML = `<i class="fa-solid ${icon}"></i> <span>${escapeHtml(message)}</span>`;
  container.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(10px)';
    toast.style.transition = 'all 0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}
