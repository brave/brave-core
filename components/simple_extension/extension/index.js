async function fetchData() {
    const record = {"date":"2022-05-20","areaName":"United Kingdom","areaCode":"K02000001","confirmedRate":33151.9,"latestBy":6338,"confirmed":22238713,"deathNew":87,"death":177977,"deathRate":265.3};
    document.getElementById("date").innerHTML=record.date;
    document.getElementById("areaName").innerHTML=record.areaName;
    document.getElementById("latestBy").innerHTML=record.latestBy;
    document.getElementById("deathNew").innerHTML=record.deathNew;
}
fetchData();