document.addEventListener('DOMContentLoaded', () => {
    console.log("Blackbox Air-Gapped Dashboard Initialized.");
    setInterval(() => {
        fetch('https://127.0.0.1:8443')
            .then(res => res.json())
            .then(data => {
                document.getElementById('status').innerText = data.status.toUpperCase();
            })
            .catch(err => console.log("API offline"));
    }, 2000);
});