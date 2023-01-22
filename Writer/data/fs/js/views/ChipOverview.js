import AbstractView from "./AbstractView.js";

function addCell(tr, text, classes) {
    var td = tr.insertCell();
    td.innerHTML = text;
    if (classes) {
        td.classList.add(classes);
    }
    return td;
}

export default class extends AbstractView {
    constructor(params, functions, payload) {
        super(params);
        this.payload = payload;
        this.setTitle("Chip Overview");
    }

    async getHtml() {
        let error = "";
        if (this.payload?.error != undefined) {
            error = `
                <div class="alert">
                    <span class="closebtn" onclick="this.parentElement.style.display='none';">&times;</span> 
                    <strong>Error!</strong> ${this.payload.error}
                </div>
            `
        }
        return error + `
            <div class="chip-overview container">
                <table class="chips-table">
                    <colgroup>
                        <col span="1" style="width: 6%;">
                        <col span="1" style="width: 6%;">
                        <col span="1" style="width: 82%;">
                        <col span="1" style="width: 6%;">
                    </colgroup>

                    <thead>
                        <tr>
                            <td>#</td>
                            <td>Type</td>
                            <td>Name</td>
                            <td></td>
                        </tr>
                    </thead>
                    <tbody>
                    </tbody>
                </table>
            </div>
        `;
    }

    onCreated(params, functions, payload) {
        const table = document.querySelector(".chip-overview table.chips-table tbody");

        /* const refreshTable = (chips) => {
            fetch(window.location.protocol + "//" + window.location.host + "/api/chips", {method: 'GET'})
                    .then(result => result.json())
                    .then(json => {
                        let newTableBody = document.createElement('tbody');
                        json.chips.forEach((item, i) => {
                            var row = newTableBody.insertRow();
                            addCell(row, i+1);
                            addCell(row, `<i class="fa-solid fa-${item.isCard ? "credit-card" : "mobile-screen-button"}"></i>`, "device-type");
                            addCell(row, item.name);
                            var deleteCell = addCell(row, "<i class=\"fa-solid fa-trash\"></i>", "delete-action");
                            deleteCell.addEventListener("click", () => {
                                fetch(window.location.protocol + "//" + window.location.host + "/api/chip/" + item.name, {method: 'DELETE'})
                                    .then((response) => {
                                        if (response.status == 200) {
                                            return response;
                                        }
                                        throw new Error();
                                    })
                                    .then(() => {
                                        refreshTable();
                                    })
                                    .catch((error) => {
                                        functions.refresh({error: "Could not delete device - try again"});
                                    })
                                
                            })
                        });
                    }
        } */

        fetch(window.location.protocol + "//" + window.location.host + "/api/chips", {method: 'GET'})
                .then(result => result.json())
                .then(json => {
                    return {chips: [].concat(json.desfire?.map(i => {
                        return {uid: i[0], name: i[1], isCard: 1}
                    })).concat(json.android?.map(i => {
                        return {uid: i[0], name: i[1], isCard: 0}
                    }))}
                })
                .then(json => {
                    json.chips.forEach((item, i) => {
                        var row = table.insertRow();
                        addCell(row, i+1);
                        addCell(row, `<i class="fa-solid fa-${item.isCard ? "credit-card" : "mobile-screen-button"}"></i>`, "device-type");
                        addCell(row, item.name);
                        var deleteCell = addCell(row, "<i class=\"fa-solid fa-trash\"></i>", "delete-action");
                        deleteCell.addEventListener("click", () => {
                            fetch(window.location.protocol + "//" + window.location.host + "/api/chip", {method: 'DELETE', body: JSON.stringify({uid: item.uid})})
                                .then((response) => {
                                    if (response.status == 200) {
                                        return response;
                                    }
                                    throw new Error();
                                })
                                .then(() => {
                                    functions.refresh();
                                })
                                .catch((error) => {
                                    functions.refresh({error: "Could not delete device - try again"});
                                })
                            
                        })
                    });
                });
    }
}
