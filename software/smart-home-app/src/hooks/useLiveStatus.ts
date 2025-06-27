import { useState, useEffect } from "react";
import { getStatus } from "../api/smartHomeApi";
import { mockStatus } from "../api/mockData";

export default function useLiveStatus(interval = 2000) {
  const [data, setData] = useState<any>(null);
  const [isMock, setIsMock] = useState(false);

  useEffect(() => {
    let mounted = true;
    async function fetchData() {
      try {
        const res = await getStatus();
        if (mounted) {
          setData(res.data);
          setIsMock(false);
        }
      } catch (e) {
        // Dacă nu reușește, folosește mock
        if (mounted) {
          setData(mockStatus);
          setIsMock(true);
        }
      }
    }
    fetchData();
    const timer = setInterval(fetchData, interval);
    return () => {
      mounted = false;
      clearInterval(timer);
    };
  }, [interval]);

  return { data, isMock };
}