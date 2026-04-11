/* Re-inject project version below the logo in the title area */
window.addEventListener('DOMContentLoaded', function() {
    var versionEl = document.getElementById('projectnumber');
    if (!versionEl) return;
    var version = versionEl.innerText;
    if (!version) return;
    var titleTable = document.querySelector('#titlearea table');
    if (!titleTable) return;
    var row = titleTable.insertRow(1);
    var cell = row.insertCell(0);
    cell.innerHTML = '<div id="projectversion">' + version + '</div>';
});
