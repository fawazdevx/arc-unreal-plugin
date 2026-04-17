// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "@openzeppelin/contracts/token/ERC721/extensions/ERC721URIStorage.sol";
import "@openzeppelin/contracts/access/Ownable.sol";

contract GameNFT is ERC721URIStorage, Ownable {
    uint256 private _tokenIdCounter;

    event GameNFTMinted(address indexed to, uint256 indexed tokenId, string tokenURI);

    constructor(address initialOwner)
        ERC721("Arc Game NFT", "ARCNFT")
        Ownable(initialOwner)
    {
        _tokenIdCounter = 1;
    }

    /**
     * @dev Mint an NFT to `to` with metadata URI `tokenURI_`.
     * Only owner (e.g., game backend) can mint.
     */
    function mintTo(address to, string calldata tokenURI_) external onlyOwner returns (uint256) {
        require(to != address(0), "Invalid recipient");

        uint256 newTokenId = _tokenIdCounter;
        _safeMint(to, newTokenId);
        _setTokenURI(newTokenId, tokenURI_);

        emit GameNFTMinted(to, newTokenId, tokenURI_);

        _tokenIdCounter += 1;
        return newTokenId;
    }

    function nextTokenId() external view returns (uint256) {
        return _tokenIdCounter;
    }
}