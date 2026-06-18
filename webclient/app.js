const state = {
  token: null,
  username: "",
  currentPath: "/",
  entries: [],
  filterTerm: "",
};

const selectors = {
  status: document.getElementById("statusBadge"),
  heroStatus: document.getElementById("heroStatus"),
  heroRoot: document.getElementById("heroRoot"),
  heroEntries: document.getElementById("heroEntries"),
  userBadge: document.getElementById("userBadge"),
  authPanel: document.getElementById("authPanel"),
  explorerPanel: document.getElementById("explorerPanel"),
  breadcrumb: document.getElementById("pathBreadcrumb"),
  entryCount: document.getElementById("entryCount"),
  fileRows: document.getElementById("fileRows"),
  previewMeta: document.getElementById("previewMeta"),
  previewContent: document.getElementById("previewContent"),
  loginForm: document.getElementById("loginForm"),
  folderForm: document.getElementById("folderForm"),
  fileForm: document.getElementById("fileForm"),
  refreshBtn: document.getElementById("refreshBtn"),
  logoutBtn: document.getElementById("logoutBtn"),
  upBtn: document.getElementById("upBtn"),
  searchInput: document.getElementById("searchInput"),
};

function setStatus(text) {
  selectors.status.textContent = text;
  if (selectors.heroStatus) {
    selectors.heroStatus.textContent = text;
  }
}

function toggleWorkspace(enabled) {
  selectors.authPanel.classList.toggle("hidden", enabled);
  selectors.explorerPanel.classList.toggle("hidden", !enabled);
  selectors.refreshBtn.disabled = !enabled;
  selectors.logoutBtn.disabled = !enabled;
  selectors.upBtn.disabled = !enabled;
}

function normalizePath(path) {
  if (!path || path.length === 0) {
    return "/";
  }
  if (!path.startsWith("/")) {
    return `/${path}`;
  }
  if (path.length > 1 && path.endsWith("/")) {
    return path.replace(/\/+$/, "");
  }
  return path;
}

function joinPath(segment) {
  const cleanSegment = segment.replace(/^[\/]+/, "");
  if (!cleanSegment) {
    return state.currentPath;
  }
  if (state.currentPath === "/") {
    return `/${cleanSegment}`;
  }
  return `${state.currentPath.replace(/\/$/, "")}/${cleanSegment}`;
}

function parentPath(path) {
  if (!path || path === "/") {
    return "/";
  }
  const parts = path.split("/").filter(Boolean);
  parts.pop();
  return parts.length ? `/${parts.join("/")}` : "/";
}

async function api(endpoint, payload = {}, authenticated = true) {
  const body = authenticated ? { token: state.token, ...payload } : payload;
  const response = await fetch(endpoint, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });

  const data = await response.json().catch(() => ({}));
  if (!response.ok || data.status === "error") {
    const reason = data.message || `HTTP ${response.status}`;
    throw new Error(reason);
  }
  return data;
}

function escapeHtml(value) {
  if (!value && value !== 0) {
    return "";
  }
  return String(value).replace(/[&<>"']/g, (char) => {
    switch (char) {
      case "&":
        return "&amp;";
      case "<":
        return "&lt;";
      case ">":
        return "&gt;";
      case '"':
        return "&quot;";
      case "'":
        return "&#39;";
      default:
        return char;
    }
  });
}

function getVisibleEntries() {
  if (!state.filterTerm) {
    return state.entries;
  }
  const term = state.filterTerm;
  return state.entries.filter((entry) =>
    entry.name.toLowerCase().includes(term),
  );
}

function updateHeroStats(visibleCount) {
  if (selectors.heroRoot) {
    selectors.heroRoot.textContent = state.currentPath;
  }
  if (selectors.heroEntries) {
    const total = state.entries.length;
    const visible = visibleCount ?? total;
    selectors.heroEntries.textContent = state.filterTerm
      ? `${visible}/${total}`
      : `${total}`;
  }
}

function renderEntries() {
  const visibleEntries = getVisibleEntries();
  if (selectors.entryCount) {
    selectors.entryCount.textContent = state.filterTerm
      ? `${visibleEntries.length} of ${state.entries.length} shown`
      : `${state.entries.length} entries`;
  }

  if (visibleEntries.length === 0) {
    selectors.fileRows.innerHTML = `
      <tr>
        <td class="table-empty" colspan="5">
          ${state.entries.length ? "No entries match the current filter." : "No files yet. Create one to get started."}
        </td>
      </tr>`;
    updateHeroStats(0);
    return;
  }

  selectors.fileRows.innerHTML = visibleEntries
    .map((entry) => {
      const bytes = entry.type === "dir" ? "—" : formatBytes(entry.size);
      const timestamp = entry.modified
        ? new Date(entry.modified * 1000).toLocaleString()
        : "";
      const encodedName = encodeURIComponent(entry.name);
      const safeLabel = escapeHtml(entry.name);
      const actions =
        entry.type === "dir"
          ? `<button class="chip-btn" data-action="open" data-name="${encodedName}" data-type="${entry.type}">Open</button>`
          : `<button class="chip-btn" data-action="preview" data-name="${encodedName}" data-type="${entry.type}">Preview</button>`;

      return `
        <tr>
          <td data-label="Name">
            <button class="link-btn" data-action="open" data-name="${encodedName}" data-type="${entry.type}">
              ${safeLabel}
            </button>
          </td>
          <td data-label="Type">${entry.type}</td>
          <td data-label="Size">${bytes}</td>
          <td data-label="Modified">${timestamp}</td>
          <td data-label="Actions" class="actions-cell">
            ${actions}
            <button class="chip-btn" data-action="delete" data-name="${encodedName}" data-type="${entry.type}">Delete</button>
          </td>
        </tr>`;
    })
    .join("");

  updateHeroStats(visibleEntries.length);
}

function formatBytes(value) {
  if (typeof value !== "number" || value < 0) {
    return "—";
  }
  if (value < 1024) {
    return `${value} B`;
  }
  const units = ["KB", "MB", "GB", "TB"];
  let size = value;
  let unitIndex = 0;
  while (size >= 1024 && unitIndex < units.length - 1) {
    size /= 1024;
    unitIndex++;
  }
  return `${size.toFixed(1)} ${units[unitIndex]}`;
}

async function refreshListing(targetPath = state.currentPath) {
  if (!state.token) {
    return;
  }
  setStatus("Loading workspace");
  try {
    const data = await api("/fs/list", { path: targetPath });
    state.currentPath = normalizePath(data.path || targetPath);
    state.entries = data.entries || [];
    selectors.breadcrumb.textContent = state.currentPath;
    renderEntries();
    setStatus("Workspace ready");
  } catch (error) {
    setStatus(error.message);
    if (/Unauthorized/i.test(error.message)) {
      logout();
    }
  }
}

if (selectors.searchInput) {
  selectors.searchInput.addEventListener("input", (event) => {
    state.filterTerm = (event.target.value || "").trim().toLowerCase();
    renderEntries();
  });
}

selectors.fileRows.addEventListener("click", async (event) => {
  const action = event.target.dataset.action;
  if (!action) {
    return;
  }
  const encodedName = event.target.dataset.name || "";
  const name = decodeURIComponent(encodedName);
  const type = event.target.dataset.type;
  const targetPath = joinPath(name);

  if (action === "open" && type === "dir") {
    await refreshListing(targetPath);
  } else if (action === "preview") {
    await previewFile(targetPath);
  } else if (action === "delete") {
    const confirmDelete = window.confirm(`Delete ${name}?`);
    if (!confirmDelete) {
      return;
    }
    try {
      await api("/fs/delete", { path: targetPath });
      await refreshListing();
      setStatus("Entry deleted");
    } catch (error) {
      setStatus(error.message);
    }
  } else if (action === "open" && type === "file") {
    await previewFile(targetPath);
  }
});

async function previewFile(path) {
  setStatus("Reading file");
  try {
    const data = await api("/fs/read", { path });
    selectors.previewMeta.textContent = `${data.bytes} bytes`;
    selectors.previewContent.textContent = data.content || "";
    setStatus("Preview ready");
  } catch (error) {
    selectors.previewMeta.textContent = "Preview failed";
    selectors.previewContent.textContent = error.message;
    setStatus(error.message);
  }
}

selectors.loginForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = new FormData(selectors.loginForm);
  const username = form.get("username");
  const password = form.get("password");
  setStatus("Authenticating");
  try {
    const data = await api("/auth/login", { username, password }, false);
    state.token = data.token;
    state.username = data.user;
    state.currentPath = "/";
    state.filterTerm = "";
    selectors.userBadge.textContent = data.user;
    selectors.loginForm.reset();
    selectors.previewMeta.textContent = "Select a file to inspect";
    selectors.previewContent.textContent = "";
    if (selectors.searchInput) {
      selectors.searchInput.value = "";
    }
    toggleWorkspace(true);
    await refreshListing("/");
  } catch (error) {
    setStatus(error.message || "Login failed");
  }
});

selectors.folderForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const data = new FormData(selectors.folderForm);
  const name = data.get("folder");
  if (!name) {
    return;
  }
  try {
    await api("/fs/mkdir", { path: joinPath(name) });
    selectors.folderForm.reset();
    await refreshListing();
    setStatus("Folder created");
  } catch (error) {
    setStatus(error.message);
  }
});

selectors.fileForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const data = new FormData(selectors.fileForm);
  const name = data.get("filename");
  const contents = data.get("contents");
  if (!name) {
    return;
  }
  try {
    await api("/fs/write", { path: joinPath(name), content: contents });
    await refreshListing();
    setStatus("File saved");
  } catch (error) {
    setStatus(error.message);
  }
});

selectors.refreshBtn.addEventListener("click", async () => {
  await refreshListing();
});

selectors.logoutBtn.addEventListener("click", () => {
  logout();
});

selectors.upBtn.addEventListener("click", async () => {
  const target = parentPath(state.currentPath);
  await refreshListing(target);
});

function logout() {
  state.token = null;
  state.username = "";
  state.entries = [];
  state.currentPath = "/";
  state.filterTerm = "";
  selectors.userBadge.textContent = "Guest";
  selectors.breadcrumb.textContent = "/";
  selectors.entryCount.textContent = "0 entries";
  selectors.fileRows.innerHTML = "";
  selectors.previewMeta.textContent = "Select a file to inspect";
  selectors.previewContent.textContent = "";
  if (selectors.searchInput) {
    selectors.searchInput.value = "";
  }
  updateHeroStats(0);
  toggleWorkspace(false);
  setStatus("Offline");
}

logout();
