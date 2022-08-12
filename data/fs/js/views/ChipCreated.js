import AbstractView from "./AbstractView.js";

export default class extends AbstractView {
    constructor(params, functions, payload) {
        super(params);
        this.setTitle("Chip created");
    }

    async getHtml() {
        return `
            <div class="chip-created container">
                <svg class="checkmark" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 52 52">
                    <circle class="checkmark__circle" cx="26" cy="26" r="25" fill="none"/>
                    <path class="checkmark__check" fill="none" d="M14.1 27.2l7.1 7.2 16.7-16.8"/>
                </svg>


                <h2>
                    NFC-Device created!
                </h2>

                <div class="buttons">
                    <button id="add-card">Add another card</button>
                    <button id="to-home">Return to homepage</button>
                </div>
            </div>
        `;
    }

    onCreated(params, functions, payload) {
        const addCard = document.querySelector("#add-card");
        const toHome = document.querySelector("#to-home");

        addCard.addEventListener("click", () => {
            functions.navigateTo(window.location.protocol + "//" + window.location.host + "/new");
        });
        toHome.addEventListener("click", () => {
            functions.navigateTo(window.location.protocol + "//" + window.location.host + "/");
        });
    }
}